#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <windows.h>
#include <string.h>

#include "../lib/STB/stb_image.h"
#include "../lib/STB/stb_image_resize2.h"

#define EXIT_WARNING			-1

#define VALID_IMAGE_EXTENTIONS	{".png",".jpeg",".jpg"}
#define VALID_VIDEO_EXTENTIONS	{".mp4"}
#define MAX_FILEEXT_SIZE		4
#define MAX_FILEPATH_SIZE		4096
#define MAX_FILENAME_SIZE		MAX_PATH
#define OUTPUT_CHANNELS			3
// I had to add backslashes here, as % and \ are operators and therefore have uses which I don't want :/
#define CHARMAP				"$@B\%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
// The light difference between every character. (255/68), not 69 because then there would be OOB errors
#define CHARMAP_WEIGHT		3.75

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define CALCULATE_LUMINESCENCE(r,g,b) (int)(0.299 * r + 0.587 * g + 0.114 * b);

/*TODO BEFORE MOVING ON TO NEW FEATURES
	- add png / variable channel support
	- add warnings to images of odd-size channel sizes (1,2,5,etc.)
*/

char* GetInputFile()
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile("res\\*", &findFileData);

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

void ExitAsWarning(char* message)
{
	printf("%s\n", message);
	system("pause");
	exit(EXIT_WARNING);
}

void ExitAsError(char* message)
{
	perror(message);
	system("pause");
	exit(EXIT_FAILURE);
}

char IsValidFileExtension(const char* fileExtension)
{
	// #defines cannot be iterated through at runtime, so this is a work around
	// It is still better to have this, as it means we can change the valid ext.s at the top of the code
	const char* validImageExtensions[] = VALID_IMAGE_EXTENTIONS;
	const char* validVideoExtensions[] = VALID_VIDEO_EXTENTIONS;
	// Check if it is an image file
	for (int ext = 0; ext < ARRAY_SIZE(validImageExtensions); ext++)
	{
		if (strcmp(fileExtension, validImageExtensions[ext]) == 0) {
			return 'i';
		}
	}
	// If it is not, check if it is a video file
	for (int ext = 0; ext < ARRAY_SIZE(validVideoExtensions); ext++)
	{
		if (strcmp(fileExtension, validVideoExtensions[ext]) == 0) {
			return 'v';
		}
	}

	return -1; // If the file ext. is not valid
}

int main()
{
	if (mkdir("res")!=0) {
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

	// Get which file to use
	char* fileName = GetInputFile();
	char programPath[MAX_FILEPATH_SIZE];
	_getcwd(programPath, sizeof(programPath));
	char filePath[MAX_FILEPATH_SIZE];
	snprintf(filePath, sizeof(filePath), "%s\\res\\%s", programPath, fileName);
	filePath[sizeof(filePath) - 1] = '\0';
	printf("Chosen file path: '%s'\n", filePath);

	// Get file type (manual for now, auto later)
	char* fileExtPtr;
	fileExtPtr = strchr(fileName, '.');
	int fileExtIndex = (int)(fileExtPtr - fileName);
	char fileExt[MAX_FILEEXT_SIZE];
	int j = 0;
	for (int i = fileExtIndex; i < strlen(fileName) && j < MAX_FILEEXT_SIZE; i++, j++) {
		fileExt[j] = fileName[i];
	}
	fileExt[j] = '\0';

	char fileType = IsValidFileExtension(fileExt);
	if (fileType == -1) {
		ExitAsWarning("The provided file extension is not supported, sorry!");
	}

	if (fileType == 'v') {
		ExitAsWarning("Video file format implementation is not completed yet, sorry!");
	}

	// Load the image file
	int originalWidth, originalHeight, channels;
	unsigned char *image = stbi_load(filePath, &originalWidth, &originalHeight, &channels, OUTPUT_CHANNELS);
	if (image == NULL){
		ExitAsError("Error loading image");
	} else {
		printf("%s", "Successfully loaded image.\n");
	}

	/*
	Chosen file path: 'D:\dev\C\Textify\res\soyboysel.jpg'
	Error loading image: Invalid argument
	*/
	// Calculate the new width based on the aspect ratio
	int newWidth = (int)((float)originalWidth / originalHeight * windowHeight * 2); // *2 because 1 char is double high than wide
	printf("Resized image size: %d wide x %d high\n", newWidth, windowHeight);

	// Allocate the memory to hold the new resized image
	unsigned char* resizedImage = (unsigned char*)malloc(newWidth * windowHeight * channels);
	if (resizedImage == NULL) {
		stbi_image_free(image);
		ExitAsError("Error allocating memory for resized image.");
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
		ExitAsError("Error resizing image");
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
