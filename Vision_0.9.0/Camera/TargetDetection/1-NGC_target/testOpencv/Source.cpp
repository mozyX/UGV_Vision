#include "Header.h"

int main(){
	bool try_again = true;
	int num_tries = 0;

	do{
		try {
			Error error;
			num_tries++;
			PrintBuildInfo();
			Mat frame;
			BusManager busMgr;
			unsigned int numCameras;

			/*******Testing********/
			struct sockaddr_in si_other;
			int s, slen = sizeof(si_other);
			WSADATA wsa;

			//Initialise winsock
			if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			{
				printf("Failed. Error Code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			//create socket
			if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
			{
				printf("socket() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			//setup address structure
			memset((char *)&si_other, 0, sizeof(si_other));
			si_other.sin_family = AF_INET;
			si_other.sin_port = htons(PORT);
			si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
			bool loop = true;
			VideoCapture capture;
			capture.open(0);
			while (loop){
				double angle=-1, distance=-1;
				capture.read(frame);
				int quit=detect_object(frame, &distance, &angle);
				if (quit == 1)
					loop = false;

				char message[sizeof(angle) + sizeof(distance)];

				memcpy(&message[0], &angle, sizeof(angle));
				memcpy(&message[8], &distance, sizeof(distance));

				//send the message
				if (sendto(s, message, sizeof(message), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
				{
					printf("sendto() failed with error code : %d", WSAGetLastError());
					exit(EXIT_FAILURE);
				}

				//puts(buf);
			}
			closesocket(s);
			WSACleanup();
			return 0;
			/*********************/

			//Get number of flycap cameras attatched
			error = busMgr.GetNumOfCameras(&numCameras);
			if (error != PGRERROR_OK)
			{
				PrintError(error);
				throw exception("Failed to Get number of cameras");
			}

			cout << "Number of cameras detected: " << numCameras << endl;

			for (unsigned int i = 0; i < numCameras; i++)
			{
				PGRGuid guid;
				error = busMgr.GetCameraFromIndex(i, &guid);
				if (error != PGRERROR_OK)
				{
					PrintError(error);
					throw exception("Failed GetCameraFromIndex");
				}
				
				RunCamera(guid);
			}
			//kills loop if exceptions keep happening
			if (num_tries < 5)
				try_again = false;

		}
		catch (bad_alloc& e) {//catches memory allocation exceptions. 
			cout << "Memory allocation failed. The program cannot continue." << endl;
			cout << "Error info: " << e.what() << endl;
			try_again = false;
			system("pause");
		}
		catch (runtime_error& e) {//catches runtime exceptions
			cout << "Runtime error occured." << endl;
			cout << "Error info: " << e.what() << endl;
			system("pause");
		}
		catch (exception& e) {//catches all other exceptions
			cout << "Exception caught. Exception: " << e.what() << endl;
			system("pause");
		}
		catch (...) {//This catches all other errors... probably 
			cout << "Unknown critical error occured. The code cannot continue." << endl;
			system("pause");
			throw;
		}
	} while (try_again);
	cout << "Done! Press Enter to exit..." << endl;
	cin.ignore();
	return 0;
}


void PrintBuildInfo()
{
	FC2Version fc2Version;
	Utilities::GetLibraryVersion(&fc2Version);

	ostringstream version;
	version << "FlyCapture2 library version: " << fc2Version.major << "." << fc2Version.minor << "." << fc2Version.type << "." << fc2Version.build;
	cout << version.str() << endl;

	ostringstream timeStamp;
	timeStamp << "Application build date: " << __DATE__ << " " << __TIME__;
	cout << timeStamp.str() << endl << endl;
}


void PrintCameraInfo(CameraInfo* pCamInfo)
{
	cout << endl;
	cout << "*** CAMERA INFORMATION ***" << endl;
	cout << "Serial number -" << pCamInfo->serialNumber << endl;
	cout << "Camera model - " << pCamInfo->modelName << endl;
	cout << "Camera vendor - " << pCamInfo->vendorName << endl;
	cout << "Sensor - " << pCamInfo->sensorInfo << endl;
	cout << "Resolution - " << pCamInfo->sensorResolution << endl;
	cout << "Firmware version - " << pCamInfo->firmwareVersion << endl;
	cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl << endl;

}


void PrintError(Error error)
{
	error.PrintErrorTrace();
}


int RunCamera(PGRGuid guid)
{
	Error error;
	Camera cam;
	CameraInfo camInfo;
	IplImage *frame;
	int user_input=0;
	Image rawImage;
	bool loop = true;
	double distance=-1, angle=-1;
	// Connect to a camera
	error = cam.Connect(&guid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		throw exception("Failed to connect to camera");
	}

	// Get the camera information
	error = cam.GetCameraInfo(&camInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		throw exception("Failed to get camera info");
	}

	PrintCameraInfo(&camInfo);

	// Start capturing images
	error = cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		throw exception("Failed to start image capture");
	}

	//Get one raw image to be able to calculate the OpenCV frame size
    cam.RetrieveBuffer(&rawImage);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
	}

	//Setting the frame size in OpenCV
	frame = cvCreateImage(cvSize(rawImage.GetCols(), rawImage.GetRows()), IPL_DEPTH_8U, 3);	

	//udp
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
	char buf[BUFLEN];
	char message[BUFLEN];
	WSADATA wsa;

	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

	while (loop)
	{
		FlyCapture2::Image convertedImage;
	
		// Retrieve an image
		error = cam.RetrieveBuffer(&rawImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			continue;
		}

		// Get the raw image dimensions
		FlyCapture2::PixelFormat pixFormat;
		unsigned int rows, cols, stride;
		rawImage.GetDimensions(&rows, &cols, &stride, &pixFormat);

		// Convert the raw image to opencv supported format
		error = rawImage.Convert(FlyCapture2::PIXEL_FORMAT_BGR, &convertedImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			throw exception("Failed to convert image from Raw");
		}

		//Copy the image into the IplImage of OpenCV
		memcpy(frame->imageData, convertedImage.GetData(), convertedImage.GetDataSize());

		//checking that there is a frame
		if (!frame)
		{
			cout << "Error: No frame." << endl;
			continue;
		}

		//detection

		//convert to Mat image
		Mat scene(frame);

		//test
		Mat scaled_frame;
		double percent = .25;
		resize(scene, scaled_frame, cvSize(scene.cols*percent, scene.rows*percent));
		int quit = detect_object(scaled_frame, &distance, &angle);
		//end test
		
		//int quit = detect_object(scene);

		//RIP
		if (quit == 1)
			loop = false;

		//udp
		BYTE array_1[8];
		BYTE array_2[8];
		BYTE array[8];

		for (int i = 0; i < sizeof(double); i++)
		{
			array_1[i] = ((BYTE*)&distance)[i];
			array_2[i] = ((BYTE*)&angle)[i];
		}

		for (int i = 0; i < (sizeof(double) * 2); i++)
		{
			if (i < sizeof(double))
				array[i] = array_1[i];

			else
				array[i] = array_2[i];
		}

		//send the message
		if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//puts(buf);
	}

	/* free memory */
	cvDestroyWindow("Original");
	cvReleaseImage(&frame);
	closesocket(s);
	WSACleanup();

	cout << "Stopping capture" << endl;

	// Stop capturing images
	error = cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		throw exception("Failed to correctly stop image capture");
	}

	// Disconnect the camera
	cout << "Disconnecting camera" << endl;
	error = cam.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		throw exception("Failed to disconnect the camera");
	}

	return 0;
}
