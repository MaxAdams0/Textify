#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <windows.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct iXY {
	int x;
	int y;
};
// Array sizes
#define MAX_CMD_SIZE 512
#define MAX_CMD_RETURN 16

// Image conversion 
#define CHARMAP " .,~-+<oiOIPBA#@" // " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@"
#define CHARMAP_SIZE 16
#define CALC_LUMINESCENCE(r,g,b) (int)(0.299 * r + 0.587 * g + 0.114 * b);

// Win32
void MaximizeConsoleWindow();
void GetConsoleSize(int *rows, int *cols);
void ClearScreen();
void HideCursor();
//int GetDirFileCount(const char *directory);
char** ListDir(const char *dir, int *fileCount);
void FreeFilePaths(char **filePaths, int fileCount);
int PrintFrame(char* filePath);

// Math
void Norm(int* v, int omax, int nmax);

// Other
char* PathConcat(const char* p1, const char* p2);
void RunCommand(char* output, const char *command, const int lineSize);
int RunFFmpegCommands(float* fps, float* duration, char* videoPath, char* frameDir, struct iXY winSize);


bool ARG_DEBUG = false;

int main(int argc, char *argv[]) {
	for (int i=0; i < argc; i++) {
		if (strcmp(argv[i], "-debug") == 0) ARG_DEBUG = true;
	}
	
	// Setup / get console size
	MaximizeConsoleWindow();
	struct iXY winSize;
	GetConsoleSize(&winSize.x, &winSize.y);
	printf("Window Size: %d x %d\n", winSize.x, winSize.y);
	
	// Get paths
	char homeDir[MAX_PATH], frameDir[MAX_PATH], videoPath[MAX_PATH];
	getcwd(homeDir, sizeof(homeDir));
	snprintf(frameDir, sizeof(frameDir), "%s\\temp_frames", homeDir);
	printf("Input video file path: ");  
	scanf("%s", videoPath);
	
	// Run & process ffmpeg commands
	int v_fCount;
	float v_fps, v_dur, v_ft;
	RunFFmpegCommands(&v_fps, &v_dur, videoPath, frameDir, winSize);
	v_ft = (1.0 / v_fps) * 1000.0; // s -> ms
	
	char **filePaths = ListDir(frameDir, &v_fCount);
	if (filePaths == NULL) {
		puts("ListDir failed, filePaths == NULL");
		return EXIT_FAILURE;
	}
	if (v_fCount <= 0) {
		printf("Could not find any files in directory '%s'... (ffmpeg command failed? Try '-debug')", frameDir);
		return EXIT_FAILURE;
	}
	printf("FPS: %05f, Frame Time: %05fms, Frame Count: %d, Duration: %05fs\n", v_fps, v_ft, v_fCount, v_dur);
	
	puts("Video will begin in 3 seconds...");
	Sleep(3000);
	ClearScreen();
	HideCursor();
	
	clock_t start, end;
	double cpuTimeLapsed;
	double waitTime = 0; // so output matches same fps as video
	for (int i = 0; i < v_fCount; i++) {
		start = clock();
		
		PrintFrame(filePaths[i]);
		
		end = clock();
		cpuTimeLapsed = ((double)(end - start)) / CLOCKS_PER_SEC; // ms / milliseconds
		waitTime = (v_ft - cpuTimeLapsed) * 1000.0; // microseconds
		usleep(waitTime);
	}


	// Free the dynamically allocated memory
	FreeFilePaths(filePaths, v_fCount);
	return 0;
}

// Win32

void MaximizeConsoleWindow() {
	HWND hwnd = GetConsoleWindow();
	if (hwnd) {
		ShowWindow(hwnd, SW_MAXIMIZE);
	} else {
		puts("Unable to maximize window (could not get hwnd)");
	}
}

void GetConsoleSize(int *cols, int *rows) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		*rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;  // Height (rows)
		*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;  // Width (columns)
	} else { // Failure case
		*cols = 0;
		*rows = 0;
	}
}

// ***Gives the console a behavior of 'overwriting' the screen instead of 'line-by-line' writing
void ClearScreen() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	// set cursor to top left
	COORD coord = {0, 0};
	SetConsoleCursorPosition(hConsole, coord);
}

// Prevent cursor flashing blocking image
void HideCursor() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;  // Hide the cursor
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

/* int GetDirFileCount(const char *dir) {
	WIN32_FIND_DATA data;
	HANDLE handle;
	int count = 0;

	char searchPath[MAX_PATH];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", dir);

	handle = FindFirstFile(searchPath, &data);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("Error opening directory: %s\n", dir);
		return -1;
	}

	do {
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count++;
		}
	} while (FindNextFile(handle, &data) != 0);

	FindClose(handle);
	return count;
}
*/

char** ListDir(const char *dir, int *fileCount) {
	WIN32_FIND_DATA fData;
	HANDLE fHandle;

	char searchPath[MAX_PATH];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", dir);

	fHandle = FindFirstFile(searchPath, &fData);
	if (fHandle == INVALID_HANDLE_VALUE) {
		printf("Error opening dir: '%s'\n", dir);
		printf("GetLastError() = %lu\n", GetLastError());
		return NULL;
	}

	int count = 0;
	// Allocate initial memory for file paths
	int capacity = 10;
	char **filePaths = (char **)malloc(capacity * sizeof(char *));
	if (filePaths == NULL) {
		printf("Memory allocation failed.\n");
		FindClose(fHandle);
		return NULL;
	}

	do {
		if (!(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// Ensure there's enough space in the array
			if (count == capacity) {
				capacity *= 2;
				filePaths = (char **)realloc(filePaths, capacity * sizeof(char *));
				if (filePaths == NULL) {
					printf("Memory allocation failed.\n");
					FindClose(fHandle);
					return NULL;
				}
			}

			// Allocate memory for each file path and copy the name
			filePaths[count] = (char *)malloc(MAX_PATH * sizeof(char));
			if (filePaths[count] == NULL) {
				printf("Memory allocation failed.\n");
				FindClose(fHandle);
				return NULL;
			}

			snprintf(filePaths[count], MAX_PATH, "%s\\%s", dir, fData.cFileName);
			count++;
		}
	} while (FindNextFile(fHandle, &fData) != 0);

	FindClose(fHandle);

	*fileCount = count;
	return filePaths;
}

void FreeFilePaths(char **filePaths, int fileCount) {
	for (int i = 0; i < fileCount; i++) {
		free(filePaths[i]);
	}
	free(filePaths);
}

int PrintFrame(char* filePath) {
	int width, height, chan = 0;
	unsigned char *img = stbi_load(filePath, &width, &height, &chan, 1); // force grayscale (1 channel)
	if (img == NULL){
		perror("Error loading image.");
		return EXIT_FAILURE;
	}
	
	char ascii[width * height];
	for (int y=0; y < height; y++) {
		for (int x=0; x < width; x++) {
			int pIndex = y * width + x;
			int val = img[pIndex];
			Norm(&val, 255, CHARMAP_SIZE-1);
			char c = CHARMAP[val];
			ascii[y * width + x] = c;
		}
		ascii[y * width + width-1] = '\n';
	}
	ClearScreen();
	ascii[(height-1) * width + (width-1)] = '\0'; //magic (causes 1 char to be missed on bottom and right but whatever) (TODO?)
	fputs(ascii, stdout);
	fflush(stdout);
	
	stbi_image_free(img);
	return EXIT_SUCCESS;
}

// Math

void Norm(int *v, int omax, int nmax) {
	int weight = omax / nmax;
	*v /= weight;
}

// Other

char* PathConcat(const char* p1, const char* p2) {
	static char buffer[MAX_PATH];
	snprintf(buffer, sizeof(buffer), "%s%s", p1, p2);
	return buffer;
}

void RunCommand(char* output, const char *command, const int lineSize) {
	FILE *fp;
	char buffer[lineSize];

	fp = popen(command, "r"); // treating console as file
	if (fp == NULL) {
		perror("popen failed");
		return;
	}

	// read output line-by-line
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		strcat(output, buffer);
	}

	pclose(fp);
}

int RunFFmpegCommands(float* fps, float* duration, char* videoPath, char* frameDir, struct iXY winSize) {
	// Fill commands
	char fpsCmd[MAX_CMD_SIZE], durCmd[MAX_CMD_SIZE], genCmd[MAX_CMD_SIZE];
	snprintf(fpsCmd, sizeof(fpsCmd), "ffprobe -v quiet -select_streams v -of default=noprint_wrappers=1:nokey=1 -show_entries stream=r_frame_rate \"%s\"", videoPath);
	snprintf(durCmd, sizeof(durCmd), "ffprobe -v quiet -of csv=p=0 -show_entries format=duration \"%s\"", videoPath);
	snprintf(genCmd, sizeof(genCmd), "ffmpeg -v quiet -i \"%s\" -vf scale=%d:%d,format=gray -pix_fmt gray -vsync 0 -y \"%s\\frame_%%07d.bmp\"", videoPath, winSize.x, winSize.y, frameDir);
	
	// Run commands
	char fpsRes[MAX_CMD_RETURN], durRes[MAX_CMD_RETURN], genRes[MAX_CMD_RETURN];
	puts("Retrieving video fps...");
	if (ARG_DEBUG) puts(fpsCmd); // DEBUG
	RunCommand(fpsRes, fpsCmd, winSize.x);
	if (ARG_DEBUG) puts(fpsRes); // DEBUG
	
	puts("Retrieving video duration...");
	if (ARG_DEBUG) puts(durCmd); // DEBUG
	RunCommand(durRes, durCmd, winSize.x);
	if (ARG_DEBUG) puts(durRes); // DEBUG
	
	puts("Converting video into frames... (this may take a while)");
	if (ARG_DEBUG) puts(genCmd); // DEBUG
	RunCommand(genRes, genCmd, winSize.x);
	if (ARG_DEBUG) puts(genRes); // DEBUG
	
	// Convert ffmpeg 'fps' response -> float value
	float num, denom;
	if (sscanf(fpsRes, "%f/%f", &num, &denom) != 2) {
		puts("Error processing fps ouput.");
		return EXIT_FAILURE;
	}
	*fps = num / denom;
	*duration = atof(durRes); // Convert ffmpeg 'duration' response -> float value
	
	return EXIT_SUCCESS;
}