#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <windows.h>
#include <string.h>

#include "../lib/STB/stb_image.h"
#include "../lib/STB/stb_image_resize2.h"

#define MAX_FILEPATH_SIZE	4096
#define MAX_FILENAME_SIZE	MAX_PATH
#define OUTPUT_CHANNELS		3
// I had to add backslashes here, as % and \ are operators and therefore have uses which I don't want :/
#define CHARMAP				"$@B\%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
// The light difference between every character. (255/68), not 69 because then there would be OOB errors
#define CHARMAP_WEIGHT		3.75

#define CALCULATE_LUMINESCENCE(r,g,b) (int)(0.299 * r + 0.587 * g + 0.114 * b);

/*TODO BEFORE MOVING ON TO NEW FEATURES
	- add png / variable channel support
	- add warnings to images of odd-size channel sizes (1,2,5,etc.)
*/


char* GetInputFile()
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile("res\\input\\*", &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		perror("Error opening directory");
		exit(EXIT_FAILURE);
	}

	int fileIndex = 0;
	char menuElement[MAX_FILENAME_SIZE];

	char** menuElements = NULL;

	do {
		// checking if the item in the directory is not a directory itself
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			strncpy(menuElement, findFileData.cFileName, MAX_FILENAME_SIZE);
			menuElement[MAX_FILENAME_SIZE - 1] = '\0';
			// reminder that ensure null-termination ^ is a C standard to indicate the end of a string

			// resize the array of strings
			menuElements = realloc(menuElements, (fileIndex + 1) * sizeof(char*));

			// allocate memory for the file name
			menuElements[fileIndex] = malloc(strlen(menuElement) + 1);
			strcpy(menuElements[fileIndex], menuElement);

			fileIndex++;
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	// close the windows search handle
	FindClose(hFind);

	// display every file in the directory
	for (int i = 0; i < fileIndex; i++) {
		printf("%d %s\n", i, menuElements[i]);
	}
	// select one of the displayed files
	int selection;
	printf("What file do you choose?: ");
	scanf("%d", &selection);

	// allocate memory for the chosen file and copy it
	char* chosenFile = malloc(strlen(menuElements[selection]) + 1);
	strcpy(chosenFile, menuElements[selection]);

	// free allocated memory
	for (int i = 0; i < fileIndex; i++) {
		free(menuElements[i]);
	}
	free(menuElements);

	return chosenFile;
}

int main()
{
	if (mkdir("res")!=0 || mkdir("res\\input")!=0 || mkdir("res\\output")!=0) {
		perror("Error creating directory");
	}
	// Maximize window
	HWND consoleWindow = GetConsoleWindow();
	ShowWindow(consoleWindow, SW_MAXIMIZE);

	// Get the size of the window
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	printf("Terminal size: %d wide x %d high\n", windowWidth, windowHeight);

	// Get which file to use for the image
	char* filename = GetInputFile();
	char programPath[MAX_FILEPATH_SIZE];
	_getcwd(programPath, sizeof(programPath));
	char filePath[MAX_FILEPATH_SIZE];
	snprintf(filePath, sizeof(filePath), "%s\\res\\input\\%s", programPath, filename);
	printf("Chosen file path: '%s'\n", filePath);

	// Load the image file
	int originalWidth, originalHeight, channels;
	unsigned char *image = stbi_load(filePath, &originalWidth, &originalHeight, &channels, OUTPUT_CHANNELS);
	if (image == NULL){
		perror("Error loading image.");
		system("pause");
		return -1;
	} else {
		printf("%s", "Successfully loaded image.\n");
	}
	// Calculate the new width based on the aspect ratio
	int newWidth = (int)((float)originalWidth / originalHeight * windowHeight * 2); // *2 because 1 char is double high than wide
	printf("Resized image size: %d wide x %d high\n", newWidth, windowHeight);

	// Allocate the memory to hold the new resized image
	unsigned char* resizedImage = (unsigned char*)malloc(newWidth * windowHeight * channels);
	if (resizedImage == NULL) {
		perror("Error allocating memory for resized image.");
		stbi_image_free(image);
		system("pause");
		return -1;
	} else {
		printf("%s", "Successfully allocated memory to resize image.\n");
	}

	// Resize the loaded file to fit the size of the window while maintaining the aspect ratio
	stbir_resize(
		image, originalWidth, originalHeight, 0,
		resizedImage, newWidth, windowHeight, 0,
		STBIR_RGB, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT
	);
	if (resizedImage == NULL){
		perror("Error resizing image.");
	} else {
		printf("%s", "Successfully resized image.\n");
	}

	system("cls");

	// Loop through the image, convert the color values to an ASCII character, and print it
	for (int h = 0; h < windowHeight; h++)
	{
		for (int w = 0; w < newWidth; w++)
		{
			int index = channels * (h * newWidth + w);
			int red = resizedImage[index + 0];
			int green = resizedImage[index + 1];
			int blue = resizedImage[index + 2];
			int luminescence = CALCULATE_LUMINESCENCE(red, green, blue);
			// Basically normalizing the luminescence value (0-255) to the bounds of the CHARMAP (0-68)
			char pixelChar = CHARMAP[(int)(luminescence / CHARMAP_WEIGHT)];
			putchar(pixelChar);
		}
		putchar('\n');
	}

	stbi_image_free(image);
	free(resizedImage);
	system("pause");
	return 0;
}
