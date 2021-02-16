
/*
 * Name:
 *   protocol_test.cpp
 *
 * Purpose:
 *   Test the C library interface to the PANGU network protocol.
 *
 * Description:
 *
 * Language:
 *   ANSI C++
 *
 * Author:
 *   Martin N Dunstan (mdunstan@computing.dundee.ac.uk)
 *   {add_authors_here}
 *
 * Copyright:
 *   (c) Space Technology Centre, University of Dundee, 2001-2016.
 *
 * History:
 *   14-Nov-2002 (mnd):
 *      Original version.
 *   {add_changes_here}
 *
 * Future work:
 *   {add_suggestions_here}
 *
 * Bugs:
 *   {add_new_bugs_here}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "socket_stuff.h"
#include "pan_protocol_lib.h"

#define SERVER_NAME	"localhost"
//#define SERVER_NAME	"192.0.0.10"
#define SERVER_PORT	10363


static int simple_tests(SOCKET);
static int general_tests(SOCKET);
static int elevation_test(SOCKET);
static int lookup_test(SOCKET);
static int scan_test(SOCKET);
static unsigned long hostid_to_address(char *);
static int get_and_save_image(SOCKET, char *);


int
main(int argc, char *argv[])
{
	long addr;
	int result = 0;
	SOCKET sock;
	unsigned long saddr_len;
	struct sockaddr_in saddr;
	const char *what = "PANGU Protocol Library Tester";
	const char *who = "Martin N Dunstan";

	/* Introduce ourselves */
	(void)fputs(what, stderr);
	(void)fputs(" by ", stderr);
	(void)fputs(who, stderr);
	(void)fputs("\n", stderr);

#ifdef _WIN32
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData))
	{
		(void)fprintf(stderr, "Failed to initialise winsock 1.1\n");
		return 1;
	}
#endif

	/* First get the numeric IP address of the server */
	addr = hostid_to_address((char *)SERVER_NAME);

	/* Create a communications TCP/IP socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		(void)fprintf(stderr, "Error: failed to create socket.\n");
		return 2;
	}

	/* Connect the socket to the remote server */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = addr;
	saddr.sin_port = htons(SERVER_PORT);
	saddr_len = sizeof(struct sockaddr_in);
	if (connect(sock, (struct sockaddr *)&saddr, saddr_len) == -1)
	{
		const char *fmt = "Error: failed to connect to %s:%d\n";
		(void)fprintf(stderr, fmt, SERVER_NAME, SERVER_PORT);
		return 2;
	}

	/* Start the PANGU network communications protocol */
	pan_protocol_start(sock);

	/* Try each test function */
	(void)(!result && (result = simple_tests(sock)));
	(void)(!result && (result = general_tests(sock)));
	(void)(!result && (result = elevation_test(sock)));
	(void)(!result && (result = lookup_test(sock)));
	(void)(!result && (result = scan_test(sock)));

	/* Terminate the PANGU network communications protocol */
	pan_protocol_finish(sock);
	SOCKET_CLOSE(sock);

#ifdef _WIN32
	WSACleanup();
#endif

	(void)printf("Finished - press ENTER to exit.\n");
	(void)getchar();
	return result;
}


static int
simple_tests(SOCKET sock)
{
	int i, status;
	float x, y, z, yaw, pitch, roll;
	char fname[1024];

	/* Initialise the camera position */
	x = 192.257f, y = 192.257f, z = 126.785f;
	yaw = 135.0f, pitch = -25.0f, roll = 0.0f;

	/* Define the field of view we want to use */
	pan_protocol_set_field_of_view(sock, 30.0);

	/* Instruct the viewer to use this position */
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Fly towards the model with constant attitude */
	for (i = 0; i < 5; i++)
	{
		/* Set the new position */
		x -= 1.9f, y -= 1.9f, z -= 1.2f;
		pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

		/* Get this image */
		(void)sprintf(fname, "zoom_%03d.ppm", i);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Change the pitch */
	for (i = 0; i < 5; i++)
	{
		/* Set the new position */
		pitch -= 2;
		pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

		/* Get this image */
		(void)sprintf(fname, "pitch_%03d.ppm", i);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Change the roll */
	for (i = 0; i < 5; i++)
	{
		/* Set the new position */
		roll += 2;
		pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

		/* Get this image */
		(void)sprintf(fname, "roll_%03d.ppm", i);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Change the yaw */
	for (i = 0; i < 5; i++)
	{
		/* Set the new position */
		yaw += 2;
		pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

		/* Get this image */
		(void)sprintf(fname, "yaw_%03d.ppm", i);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Success */
	return 0;
}


static int
general_tests(SOCKET sock)
{
	int i, status;
	char valid;
	float fov, elev;
	float pix_x, pix_y;
	float px, py, pz;
	float x, y, z, yaw, pitch, roll;
	char fname[1024];

	/* Initialise the camera position */
	x = 192.257f, y = 192.257f, z = 126.785f;
	yaw = 135.0f, pitch = -25.0f, roll = 0.0f;

	/* Instruct the viewer to use this position */
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Retrieve views of varying field-of-view angles */
	for (fov = 5.0f; fov < 90.0f; fov *= 2.0f)
	{
		/* Set the field of view width */
		pan_protocol_set_field_of_view(sock, fov);

		/* Get this image */
		(void)sprintf(fname, "fov_%03d.ppm", (int)fov);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Define the field of view we want to use */
	pan_protocol_set_field_of_view(sock, 30.0);

	/* Try other sky types at a different angle */
	pitch = -10.0f;
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);
	for (i = 0; i < 6; i++)
	{
		pan_protocol_set_sky_type(sock, i);
		(void)sprintf(fname, "sky_%d.ppm", (int)i);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}

	/* Try a different Sun colour */
	pan_protocol_set_sun_colour(sock, 1.0, 1.0, 0.0);
	status = get_and_save_image(sock, (char *)"orange_sun.ppm");
	if (status) return status;

	/* Try a different surface colour */
	pan_protocol_set_sun_colour(sock, 0.0, 1.0, 0.0);
	status = get_and_save_image(sock, (char *)"green_planet.ppm");
	if (status) return status;

	/* Current camera status */
	(void)printf("Camera position = (%f, %f, %f)\n", x, y, z);
	(void)printf("Camera attitude = (yaw %f, pitch %f, roll %f\n", yaw, pitch, roll);

	/* Find out how high we are above the surface */
	elev = pan_protocol_get_elevation(sock, &valid);
	if (valid)
		(void)printf("Elevation above the surface = %fm\n", elev);
	else
		(void)printf("Camera is not directly above the surface\n");

	/* Find the 3D position directly below us */
	pan_protocol_get_point(sock, 0.0, 0.0, -1.0, &px, &py, &pz, &valid);
	if (valid)
	{
		elev = (float)sqrt((px-x)*(px-x) + (py-y)*(py-y) + (pz-z)*(pz-z));
		(void)printf("Position on surface below camera = (%f, %f, %f)\n", px, py, pz);
		(void)printf("Elevation above the surface = %fm\n", elev);
	}
	else
		(void)printf("Camera is not directly above the surface\n");

	/* Perform back projection on the last image received */
	pix_x = 0.5f, pix_y = 0.1f;
	pan_protocol_lookup_point(sock, pix_x, pix_y, &px, &py, &pz, &valid);
	if (valid)
	{
		const char *fmt = "Pixel (%f, %f) has position (%f, %f, %f)\n";
		(void)printf(fmt, pix_x, pix_y, px, py, pz);
	}
	else
	{
		const char *fmt = "Pixel (%f, %f) is not over the surface.\n";
		(void)printf(fmt, pix_x, pix_y);
	}

	/* Check how long it takes to get 10 images */
	double time_start = 0;
	double time_finish = 0;
	unsigned long size = 0;
	pan_protocol_get_image(sock, &size);
	time_start = pan_protocol_get_time_tag(sock, NULL);
	for (int i = 0; i<10; i++)
	{
		pan_protocol_get_image(sock, &size);
	}
	time_finish = pan_protocol_get_time_tag(sock, NULL);
	const char *fmt = "Ten images takes %u microseconds to get.\n";
	(void)printf(fmt, (unsigned long)(time_finish-time_start));

	/* Change to orthographic projection */
	enum proj_mode { MODE_PERSPECTIVE=0, MODE_ORTHOGRAPHIC=1  };
	unsigned long cid = 1; // required
	pan_protocol_set_projection_mode(sock, cid, MODE_ORTHOGRAPHIC);
	pan_protocol_set_viewpoint_by_angle(sock, 850.0, 950.0, 770.0, 140.0, -30.0, 0.0);
	for (i = 0; i < 5; i++)
	{
		/* Change field of view */
		double dimension = 1000 + i*500;
		unsigned long cid = 0;
		pan_protocol_set_ortho_field_of_view(sock, cid, dimension, dimension);

		/* Get this image */
		(void)sprintf(fname, "wid_hei_%03d.ppm", (int)dimension);
		status = get_and_save_image(sock, fname);
		if (status) return status;
	}
	/* Change back to perspective projection */
	pan_protocol_set_projection_mode(sock, cid, MODE_PERSPECTIVE);
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Success */
	return 0;
}


static int
elevation_test(SOCKET sock)
{
	int i;
	float px, py, pz;
	float x, y, z, yaw, pitch, roll;
	float camera_pos[] = {
		192.257004f, 192.257004f, 126.785004f,
		 10.0f,  10.0f, 300.0f,
		 10.0f, -10.0f, 300.0f,
		-10.0f, -10.0f, 300.0f,
		-10.0f,  10.0f, 300.0f,
		  0.0f,   0.0f, 300.0f
	};
	float elevations[sizeof(camera_pos)/(3*sizeof(camera_pos[0]))];
	char valid[sizeof(elevations)/sizeof(elevations[0])];
	int nelts = sizeof(valid)/sizeof(valid[0]);

	/* Position the camera over the center of the model */
	x = 0.0f, y = 0.0f, z = 600.0f;
	yaw = 0.0f, pitch = -90.0f, roll = 0.0f;
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Compute the elevations over a small grid */
	pan_protocol_get_elevations(sock, nelts, camera_pos, elevations, valid);
	for (i = 0; i < nelts; i++)
	{
		px = camera_pos[3*i + 0];
		py = camera_pos[3*i + 1];
		pz = camera_pos[3*i + 2];
		if (!valid[i])
		{
			const char *fmt = "No elevation at <%5.1f, %5.1f, %5.1f>\n";
			(void)printf(fmt, px, py, pz);
		}
		else
		{
			const char *fmt = "Elevation at <%5.1f, %5.1f, %5.1f> = %f\n";
			(void)printf(fmt, px, py, pz, elevations[i]);
		}
	}

	/* Success */
	return 0;
}


static int
lookup_test(SOCKET sock)
{
	int i, status;
	float px, py, pz;
	float pix_x, pix_y;
	float x, y, z, yaw, pitch, roll;
	float pixel_pos[] = {
		0.5f, 0.1f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.5f, 0.5f,
		0.5f, 0.9f,
		0.0f, 0.0f,
		0.5f, 0.5f
	};
	float positions[3*sizeof(pixel_pos)/(2*sizeof(pixel_pos[0]))];
	char valid[sizeof(positions)/(3*sizeof(positions[0]))];
	int nelts = sizeof(valid)/sizeof(valid[0]);
	const char *fmt0 = "   Model under pixel <%f, %f> = <%5.1f, %5.1f, %5.1f>\n";
	const char *fmt1 = "   No model at pixel <%f, %f>\n";

	/* Initialise the camera position */
	x = 192.257f, y = 192.257f, z = 126.785f;
	yaw = 135.0f, pitch = -25.0f, roll = 0.0f;
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Black sky */
	pan_protocol_set_sky_type(sock, 0);

	/* Define the field of view we want to use */
	pan_protocol_set_field_of_view(sock, 30.0);

	/* Get an image to back project into */
	status = get_and_save_image(sock, (char *)"trash.ppm");
	if (status) return status;

	/* Perform the lookups individually */
	(void)printf("Performing multiple LookupPoint() requests:\n");
	for (i = 0; i < nelts; i++)
	{
		pix_x = pixel_pos[2*i + 0];
		pix_y = pixel_pos[2*i + 1];
		pan_protocol_lookup_point(sock, pix_x, pix_y, &px, &py, &pz, valid);
		if (valid[0])
			(void)printf(fmt0, pix_x, pix_y, px, py, pz);
		else
			(void)printf(fmt1, pix_x, pix_y);
	}

	/* Perform the lookups in one step */
	(void)printf("Performing one LookupPoints() request:\n");
	pan_protocol_lookup_points(sock, nelts, pixel_pos, positions, valid);
	for (i = 0; i < nelts; i++)
	{
		pix_x = pixel_pos[2*i + 0];
		pix_y = pixel_pos[2*i + 1];
		px = positions[3*i + 0];
		py = positions[3*i + 1];
		pz = positions[3*i + 2];
		if (valid[i])
			(void)printf(fmt0, pix_x, pix_y, px, py, pz);
		else
			(void)printf(fmt1, pix_x, pix_y);
	}

	/* Success */
	return 0;
}


static int
scan_test(SOCKET sock)
{
	int i;
	float dx, dy, dz;
	float px, py, pz;
	float x, y, z, yaw, pitch, roll;
	float view_dir[] = {
		 0.0f, 0.0f, -1.0f,
		-0.1f, 0.2f, -1.0f,
		 0.5f, 0.2f, -1.0f,
		 0.5f, 0.2f,  1.0f
	};
	float positions[sizeof(view_dir)/sizeof(view_dir[0])];
	char valid[sizeof(positions)/(3*sizeof(positions[0]))];
	int nelts = sizeof(valid)/sizeof(valid[0]);

	/* Position the camera over the center of the model */
	x = 0.0f, y = 0.0f, z = 600.0f;
	yaw = 0.0f, pitch = -90.0f, roll = 0.0f;
	pan_protocol_set_viewpoint_by_angle(sock, x, y, z, yaw, pitch, roll);

	/* Scan the model along different directions */
	pan_protocol_get_points(sock, nelts, view_dir, positions, valid);
	for (i = 0; i < nelts; i++)
	{
		dx = view_dir[3*i + 0];
		dy = view_dir[3*i + 1];
		dz = view_dir[3*i + 2];
		px = positions[3*i + 0];
		py = positions[3*i + 1];
		pz = positions[3*i + 2];
		if (!valid[i])
		{
			const char *fmt = "No model along <%f, %f, %f>\n";
			(void)printf(fmt, dx, dy, dz);
		}
		else
		{
			const char *fmt = "Model at <%5.1f, %5.1f, %5.1f> along <%f, %f, %f>\n";
			(void)printf(fmt, px, py, pz, dx, dy, dz);
		}
	}

	/* Success */
	return 0;
}


static unsigned long
hostid_to_address(char *s)
{
	struct hostent *host;

	/* Assume we have a dotted IP address ... */
	long result = inet_addr(s);
	if (result != (long)INADDR_NONE) return result;

	/* That failed so assume DNS will resolve it. */
	host = gethostbyname(s);
	return host ? *((long *)host->h_addr_list[0]) : INADDR_NONE;
}


static int
get_and_save_image(SOCKET sock, char *fname)
{
	unsigned long todo;
	unsigned char *ptr, *img;
	FILE *handle;

	/* Retrieve an image */
	(void)fprintf(stderr, "Getting image '%s'\n", fname);
	img = pan_protocol_get_image(sock, &todo);

	/* Open the output file for writing */
	handle = fopen(fname, "wb");
	if (!handle)
	{
		const char *fmt = "Error: failed to open '%s' for writing\n";
		(void)fprintf(stderr, fmt, fname);
		return 4;
	}

	/* Write the image data to the file */
	ptr = img;
	while (todo > 1024)
	{
		long wrote;
		wrote = fwrite(ptr, 1, 1024, handle);
		if (wrote < 1)
		{
			const char *fmt = "Error writing to '%s'\n";
			(void)fprintf(stderr, fmt, fname);
			(void)fclose(handle);
			return 5;
		}
		else
		{
			todo -= wrote;
			ptr += wrote;
		}
	}
	if (todo)
	{
		long wrote;
		wrote = fwrite(ptr, 1, todo, handle);
		if (wrote < 1)
		{
			const char *fmt = "Error writing to '%s'\n";
			(void)fprintf(stderr, fmt, fname);
			(void)fclose(handle);
			return 5;
		}
	}

	/* Close the file */
	(void)fclose(handle);

	/* Release the image data */
	(void)free(img);

	/* Return success */
	return 0;
}
