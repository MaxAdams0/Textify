#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct iXY {
	int x;
	int y;
};

#define CHARMAP " .,~-+<oiOIPBA#@"
// " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@"
#define CALC_LUMINESCENCE(r,g,b) (int)(0.299 * r + 0.587 * g + 0.114 * b);

int Norm(int v, int omax, int nmax);
void ClearScreen();
void HideCursor();
int GetDirFileCount(const char *directory);
char* PathConcat(const char* p1, const char* p2);
char** ListDir(const char *dir, int *fileCount);
void FreeFilePaths(char **filePaths, int fileCount);

int main() {
	HANDLE hStdout;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	struct iXY winSize; // predefined -- TODO win32 =(
	winSize.x = 237;
	winSize.y = 64;
	printf("Window Size: %d x %d\n", winSize.x, winSize.y);
	
	char homeDir[MAX_PATH] = "E:\\dev\\Textify";
	
	char framePath[MAX_PATH];
	strcpy(framePath, PathConcat(homeDir, "\\res\\blladeyblijnkey"));
	
	float fps = 24;
	float frameTime = (1.0 / fps) * 1000.0;
	int frameCount = GetDirFileCount(framePath);
	float dur = (float)(frameCount) / fps;
	printf("FPS: %05f, FT: %05f, Frames: %d, Dur: %05f\n", 
		fps, frameTime, frameCount, dur);
	
	char **filePaths = ListDir(framePath, &frameCount);
	if (filePaths == NULL) {
		return -1;
	}
	
	Sleep(2000);
	ClearScreen();
	HideCursor(); // idk jik
	
	clock_t start, end;
	double cpuTimeLapsed;
	double waitTime = 0;
	for (int i = 0; i < frameCount; i++) {
		start = clock();
		
		int chan = 0;
		unsigned char *img = stbi_load(filePaths[i], &winSize.x, &winSize.y, &chan, 3);
		if (img == NULL){
			perror("Error loading image.");
			return -1;
		}
		
		char ascii[winSize.x * winSize.y];
		for (int y=0; y < winSize.y; y++) {
			for (int x=0; x < winSize.x; x++) {
				int pIndex = chan * (y * winSize.x + x);
				
				int r = img[pIndex + 0];
				int g = img[pIndex + 1];
				int b = img[pIndex + 2];
				int lum = CALC_LUMINESCENCE(r, g, b);
				
				lum = Norm(lum, 255, strlen(CHARMAP)-1);
				char c = CHARMAP[lum];
				ascii[y * winSize.x + x] = c;
			}
			ascii[y * winSize.x + winSize.x-1] = '\n';
		}
		ClearScreen();
		ascii[(winSize.y-1) * winSize.x + (winSize.x-1)] = '\0';
		fputs(ascii, stdout);
		fflush(stdout);
		
		end = clock();
		cpuTimeLapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
		waitTime = frameTime - cpuTimeLapsed;
		
		stbi_image_free(img);
		Sleep(waitTime);
		//printf("ft: %05f, cpu: %05f, wait: %d\n", frameTime, cpuTimeLapsed, waitTime);
	}


	// Free the dynamically allocated memory
	FreeFilePaths(filePaths, frameCount);
	return 0;
}

// ffmpeg -i "./res/aot_37.mkv" -vf "scale=237:64,format=gray" -pix_fmt gray -vsync 0 -y "./res/frames/frame_%07d.bmp"

int Norm(int v, int omax, int nmax) {
	int weight = omax / nmax;
	return v / weight;
}

void ClearScreen() {
	// Get the console handle
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// Get the current console screen buffer info
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	// Set the cursor to the top-left corner
	COORD coord = {0, 0};
	SetConsoleCursorPosition(hConsole, coord);
}

void HideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;  // Hide the cursor
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

int GetDirFileCount(const char *directory) {
	WIN32_FIND_DATA fData;
	HANDLE fHandle;
	int fCount = 0;

	char searchPath[MAX_PATH];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);

	fHandle = FindFirstFile(searchPath, &fData);
	if (fHandle == INVALID_HANDLE_VALUE) {
		printf("Error opening directory: %s\n", directory);
		return -1;
	}

	do {
		if (!(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			fCount++;
		}
	} while (FindNextFile(fHandle, &fData) != 0);

	FindClose(fHandle);
	return fCount;
}

char* PathConcat(const char* p1, const char* p2) {
	static char buffer[MAX_PATH];
	snprintf(buffer, sizeof(buffer), "%s%s", p1, p2);
	return buffer;
}

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
