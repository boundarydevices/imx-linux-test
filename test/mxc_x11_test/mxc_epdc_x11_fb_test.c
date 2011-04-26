/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * @file mxc_epdc_x11_fb_test.c
 *
 * @brief MXC EPDC unit test applicationi for framebuffer updates
 * based on X11 changes to its root window
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrandr.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <sys/ioctl.h>

#define WAVEFORM_MODE_DU	0x1	/* Grey->white/grey->black */
#define WAVEFORM_MODE_GC16	0x2	/* High fidelity (flashing) */
#define WAVEFORM_MODE_GC4	0x3	/* Lower fidelity */

static int kbhit(void)
{
	struct pollfd pollfd;
	pollfd.fd = 0;
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	return poll(&pollfd, 1, 0);
}

int main (int argc, char* argv[])
{
	/* Declare and init variables used after cleanup: goto. */
	Display* xDisplay = NULL;
	Damage xDamageScreen = 0;
	int fd = -1;
	XRRScreenConfiguration* xrrScreenConfig = NULL;

	/* Open the X display */
	xDisplay = XOpenDisplay(NULL);
	if (NULL == xDisplay)
	{
		printf("\nError: unable to open X display\n");
		goto cleanup;
	}

	/* Acess the root window for the display. */
	Window xRootWindow = XDefaultRootWindow(xDisplay);
	if (0 == xRootWindow)
	{
		printf("\nError: unable to access root window for screen\n");
		goto cleanup;
	}

	/* Access the XRandR screen configuration. */
	xrrScreenConfig = XRRGetScreenInfo(xDisplay, xRootWindow);
	if (NULL == xrrScreenConfig)
	{
		printf("\nError: unable to query XRandR screen config\n");
		goto cleanup;
	}

	/* Query the list of XRandR configured screen sizes. */
	int xrrSizeListCount;
	XRRScreenSize* xrrSizeList =
		XRRConfigSizes(xrrScreenConfig, &xrrSizeListCount);

	/* Query the current XRandR screen rotation and which screen size */
	/* is currently being used. */
	Rotation xrrRotate;
	int xrrSizeIndex =
		XRRConfigCurrentConfiguration( xrrScreenConfig, &xrrRotate);
	const int screenWidth = xrrSizeList[xrrSizeIndex].width;
	const int screenHeight = xrrSizeList[xrrSizeIndex].height;

	/* Display XRandR information */
	printf("using XRandR: screen size = %dx%d, rotation = ",
		screenWidth, screenHeight);
	int fbRotate = -1;
	if (RR_Rotate_0 & xrrRotate)
	{
		printf("UR\n");
		fbRotate = FB_ROTATE_UR;
	}
	else if (RR_Rotate_90 & xrrRotate)
	{
		printf("CW\n");
		fbRotate = FB_ROTATE_CW;
	}
	else if (RR_Rotate_180 & xrrRotate)
	{
		printf("UD\n");
		fbRotate = FB_ROTATE_UD;
	}
	else if (RR_Rotate_270 & xrrRotate)
	{
		printf("CCW\n");
		fbRotate = FB_ROTATE_CCW;
	}
	else
	{
		printf("???\n");
	}
	if (-1 == fbRotate)
	{
		printf("\nError: unsupported XRandR rotation = 0x%04x\n",
			xrrRotate);
		goto cleanup;
	}

	/* Query the damage extension associated with this display. */
	int xDamageEventBase;
	int xDamageErrorBase;
	if (!XDamageQueryExtension(xDisplay, &xDamageEventBase, &xDamageErrorBase))
	{
		printf("\nError: unable to query XDamage extension\n");
		goto cleanup;
	}

	/* Setup to receive damage notification on the main screen */
	/* each time the bounding box increases until it is reset. */
	xDamageScreen =
		XDamageCreate(xDisplay, xRootWindow,
			XDamageReportBoundingBox);
	if (0 == xDamageScreen)
	{
		printf("\nError: unable to setup X damage on display screen\n");
		goto cleanup;
	}

	/* Find the EPDC FB device */
	char fbDevName[10] = "/dev/fb";
	int fbDevNum = 0;
	struct fb_fix_screeninfo fbFixScreenInfo;
	do {
		/* Close previously opened fbdev */
		if (fd >= 0)
		{
			close(fd);
			fd = -1;
		}

		/* Open next fbdev */
		fbDevName[7] = '0' + (fbDevNum++);
		fd = open(fbDevName, O_RDWR, 0);
		if (fd < 0)
		{
			printf("Error in opening fb device\n");
			perror(fbDevName);
			goto cleanup;
		}

		/* Query fbdev fixed screen info. */
		if (0 > ioctl(fd, FBIOGET_FSCREENINFO, &fbFixScreenInfo))
		{
			printf("Error in ioctl(FBIOGET_FSCREENINFFO) call\n");
			perror(fbDevName);
			goto cleanup;
		}

	} while (0 != strcmp(fbFixScreenInfo.id, "mxc_epdc_fb"));
	printf("EPDC fbdev is %s\n", fbDevName);

	/* Query fbdev var screen info. */
	struct fb_var_screeninfo fbVarScreenInfo;
	if (0 > ioctl(fd, FBIOGET_VSCREENINFO, &fbVarScreenInfo))
	{
		printf("Error in ioctl(FBIOGET_VSCREENINFO) call\n");
		perror(fbDevName);
		goto cleanup;
	}

	/* Force EPDC to initialize */
	fbVarScreenInfo.activate = FB_ACTIVATE_FORCE;
	if (0 > ioctl(fd, FBIOPUT_VSCREENINFO, &fbVarScreenInfo))
	{
		printf("Error in ioctl(FBIOPUT_VSCREENINFO) call\n");
		perror(fbDevName);
		goto cleanup;
	}

	/* Put EPDC into region update mode. */
	int mxcfbSetAutoUpdateMode = AUTO_UPDATE_MODE_REGION_MODE;
	if (0 > ioctl(fd, MXCFB_SET_AUTO_UPDATE_MODE, &mxcfbSetAutoUpdateMode))
	{
		printf("Error in ioctl(MXCFB_SET_AUTO_UPDATE_MODE) call\n");
		perror(fbDevName);
		goto cleanup;
	}

	/* Setup waveform modes. */
	struct mxcfb_waveform_modes mxcfbWaveformModes;
	mxcfbWaveformModes.mode_init = 0;
	mxcfbWaveformModes.mode_du = 1;
	mxcfbWaveformModes.mode_gc4 = 3;
	mxcfbWaveformModes.mode_gc8 = 2;
	mxcfbWaveformModes.mode_gc16 = 2;
	mxcfbWaveformModes.mode_gc32 = 2;
	if (0 > ioctl(fd, MXCFB_SET_WAVEFORM_MODES, &mxcfbWaveformModes))
	{
		printf("Error in ioctl(MXCFB_SET_WAVEFORM_MODES) call\n");
		perror(fbDevName);
		goto cleanup;
	}


	/* Common properties for EPDC screen update */
	struct mxcfb_update_data mxcfbUpdateData;
	mxcfbUpdateData.update_mode = UPDATE_MODE_FULL;
	mxcfbUpdateData.waveform_mode = WAVEFORM_MODE_AUTO;
	mxcfbUpdateData.temp = TEMP_USE_AMBIENT;
	mxcfbUpdateData.flags = 0;
	mxcfbUpdateData.update_marker = 0;

	int numPanelUpdates = 0;
	while (1)
	{
		int numDamageUpdates = 0;
		int updateLeft, updateRight;
		int updateTop, updateBottom;

		while (XPending(xDisplay))
		{
			XEvent xEvent;
			XNextEvent(xDisplay, &xEvent);
			if ((XDamageNotify+xDamageEventBase) == xEvent.type)
			{
				XDamageNotifyEvent* xDamageNotifyEvent =
					(XDamageNotifyEvent*)&xEvent;

				int left = xDamageNotifyEvent->area.x;
				int top = xDamageNotifyEvent->area.y;
				int right = left + xDamageNotifyEvent->area.width;
				int bottom = top + xDamageNotifyEvent->area.height;

				if (numDamageUpdates++ > 0)
				{
					if (left < updateLeft)
					{
						updateLeft = left;
					}
					if (right > updateRight)
					{
						updateRight = right;
					}
					if (top < updateTop)
					{
						updateTop = top;
					}
					if (bottom > updateBottom)
					{
						updateBottom = bottom;
					}
				}
				else
				{
					updateLeft = left;
					updateTop = top;
					updateRight = right;
					updateBottom = bottom;
				}
			}
		}

		if (numDamageUpdates <= 0)
		{
			continue;
		}

		/* Send accum bound rect updates to EPDC */
		mxcfbUpdateData.update_marker = ++numPanelUpdates;

		if (FB_ROTATE_UR == fbRotate)
		{
			mxcfbUpdateData.update_region.left = 0;
			mxcfbUpdateData.update_region.top = 0;

			mxcfbUpdateData.update_region.width = updateRight;
			mxcfbUpdateData.update_region.height = updateBottom;
		}
		else if (FB_ROTATE_UD == fbRotate)
		{
			mxcfbUpdateData.update_region.left = 0;
			mxcfbUpdateData.update_region.top = 0;

			mxcfbUpdateData.update_region.width =
				screenWidth - updateLeft;
			mxcfbUpdateData.update_region.height =
				screenHeight - updateTop;
		}
		else if (FB_ROTATE_CW == fbRotate)
		{
			mxcfbUpdateData.update_region.left = 0;
			mxcfbUpdateData.update_region.top = 0;

			mxcfbUpdateData.update_region.width =
				screenWidth - updateTop;
			mxcfbUpdateData.update_region.height =
				updateRight;
		}
		else if (FB_ROTATE_CCW == fbRotate)
		{
			mxcfbUpdateData.update_region.left = 0;
			mxcfbUpdateData.update_region.top = 0;

			mxcfbUpdateData.update_region.width =
				updateBottom;
			mxcfbUpdateData.update_region.height =
				screenHeight - updateLeft;
		}
		else
		{
			continue;
		}

		if (0 > ioctl(fd, MXCFB_SEND_UPDATE, &mxcfbUpdateData))
		{
			printf("Error in ioctl(MXCFB_SEND_UPDATE) call\n");
			perror(fbDevName);
			goto cleanup;
		}

		/* Display what getting updated */
		printf("%d: (x,y)=(%d,%d) (w,h)=(%d,%d)\n",
			numPanelUpdates,
			updateLeft, updateTop,
			updateRight - updateLeft,
			updateBottom - updateTop);

		/* Clear the damage update bounding box */
		XDamageSubtract(xDisplay, xDamageScreen, None, None);

		/* Wait for previous EPDC update to finish */
		if (0 > ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE,
				&mxcfbUpdateData.update_marker))
		{
			printf("Error in ioctl(MXCFB_WAIT_FOR_UPDATE_COMPLETE) call\n");
			perror(fbDevName);
			goto cleanup;
		}
	}

cleanup:
	if (fd >= 0)
	{
		close(fd);
		fd = -1;
	}

	if (NULL != xrrScreenConfig)
	{
		XRRFreeScreenConfigInfo(xrrScreenConfig);
		xrrScreenConfig = NULL;
	}

	if (0 != xDamageScreen)
	{
		XDamageDestroy(xDisplay, xDamageScreen);
		xDamageScreen = 0;
	}

	if (NULL != xDisplay)
	{
		XCloseDisplay(xDisplay);
		xDisplay = NULL;
	}

	return 0;
}
