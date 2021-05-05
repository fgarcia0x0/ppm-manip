#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#define RELEASE(ptr) do { if (ptr) free(ptr); } while (0)

typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
} RgbColor;

typedef enum
{
	PPM_MODE_P1,
	PPM_MODE_P3,
	PPM_MODE_P6
} PPM_MODE;

typedef struct
{
	uint32_t Width;
	uint32_t Heigth;
	RgbColor* Pixels;
	PPM_MODE Mode;
} PpmImage;

typedef RgbColor (*UnaryPredicate)(RgbColor color);

RgbColor DefaultAct(RgbColor color)
{
	return color;
}

/**
 * @brief      Peek next character
 *
 * @param      hStream  The I/O stream
 *
 * @return     Returns the next character in the input sequence, without
 *             extracting it: The character is left as the next character to be
 *             extracted from the stream.
 */
int fpeek(FILE* restrict hStream);

/**
 * @brief      Clear all white-spaces and comment's of stream
 *
 * @param      hFile  The I/O stream
 */
void ClearComment(FILE* restrict hFile);

/**
 * @brief      Gets pixel color in the position (x, y).
 *
 * @param[in]  x       The x-coordinate of the pixel to retrieve.
 * @param[in]  y       The y-coordinate of the pixel to retrieve.
 * @param      pImage  pointer to PpmImage Structure
 *
 * @return     A RgbColor structure that represents the color of the specified
 *             pixel.
 */
RgbColor GetPixel(uint32_t x, uint32_t y, PpmImage* pImage);

/**
 * @brief      Load PPM image format into memory
 *
 * @param[in]  filename  the image path in the system
 *
 * @return     A pointer to PpmImage Structure containing the content and
 *             information of the image
 */
PpmImage* LoadImage(const char* filename);

/**
 * @brief      Writes PPM image into archive with filename specified.
 *
 * @param[in]  filename      The filename
 * @param      pImage        pointer to PpmImage
 * @param[in]  mode          PPM Image Mode (P1, P3, P6)
 * @param[in]  pfnTransform  pointer to the function that applies the
 *                           transformations to the pixels
 */
void WriteToPpm(const char* filename, PpmImage* pImage, PPM_MODE mode, 
				UnaryPredicate pfnTransform);

/**
 * @brief      Convert PPM image to grayscale
 *
 * @param[in]  PixelColor  The pixel color
 *
 * @return     The new pixel color
 */
RgbColor ToGrayScale(RgbColor PixelColor);

/**
 * @brief      Convert PPM image to grayscale image P1
 *
 * @param[in]  PixelColor  The pixel color
 *
 * @return     The new pixel color
 */
RgbColor ToGrayScaleP3(RgbColor PixelColor);

#define LOAD_PPM_IMAGE(file) LoadImage(file)
#define WRITE_PPM_FILE(filename, ptr, mode) WriteToPpm(filename, ptr, mode, DefaultAct)
#define WRITE_PPM_GRAY_P3_FILE(filename, ptr) WriteToPpm(filename, ptr, PPM_MODE_P1, ToGrayScaleP3)
#define WRITE_PPM_GRAY_FILE(filename, ptr) WriteToPpm(filename, ptr, PPM_MODE_P6, ToGrayScale)
#define RELEASE_PPM(pImage) do { RELEASE(pImage->Pixels); RELEASE(pImage); } while(0)

int main()
{
	PpmImage* pImage = LOAD_PPM_IMAGE("../samples/sample.ppm");
	assert(pImage);

	WRITE_PPM_GRAY_FILE("resultGray.ppm", pImage);

	RELEASE_PPM(pImage);
	return 0;
}

int fpeek(FILE* restrict hStream)
{
    int c = fgetc(hStream);
    ungetc(c, hStream);
    return c;
}

void ClearComment(FILE* restrict hFile)
{
	if (!hFile)
		return;

	enum { MAX_BUFFER_SIZE = 1024 };

    int c = EOF;

    while (c = fpeek(hFile), c == '\n' || c == '\r')
        (void) fgetc(hFile);

    if (c == '#') 
    {
    	char line[MAX_BUFFER_SIZE] = {0};
        fgets(line, MAX_BUFFER_SIZE - 1, hFile);
    }
}

RgbColor GetPixel(uint32_t x, uint32_t y, PpmImage* pImage)
{
	assert(pImage);
	assert(x < pImage->Width && y < pImage->Heigth);
	return pImage->Pixels[y * pImage->Width + x];
}

PpmImage* LoadImage(const char* filename)
{
	if (!filename || !filename[0])
		return NULL;

	FILE* hFile = fopen(filename, "rb");
	if (!hFile)
		return NULL;

	ClearComment(hFile);
    int mode = 0;

    // Get Header
    char szImageSign[3] = {0};
    fgets(szImageSign, sizeof szImageSign, hFile);

    mode = szImageSign[1] - '0';

    if (mode != 3 && mode != 6)
    {
    	perror("Invalid image mode");
    	fclose(hFile);
    	return NULL;
    }

    uint32_t imgWidth = 0;
    uint32_t imgHeigth = 0;
    uint32_t maxColor = 0;

    // Get Image Width
    ClearComment(hFile);
    fscanf(hFile, "%u", &imgWidth);

    // Get Image Height
    ClearComment(hFile);
    fscanf(hFile, "%u", &imgHeigth);
    
    // Get Image max color bit
    ClearComment(hFile);
    fscanf(hFile, "%u", &maxColor);

    // Allocate image structure into memory
    PpmImage* pImage = (PpmImage*) malloc(sizeof(PpmImage));
    if (!pImage)
    {
    	fprintf(stderr, "[-] memory exausted");
    	exit(EXIT_FAILURE);
    }

    // setup image information
    pImage->Width = imgWidth;
    pImage->Heigth = imgHeigth;

    // calculate image size
    const size_t imgSize = imgWidth * imgHeigth * sizeof(RgbColor);

    // Allocate the image into memory
    RgbColor* ColorBuffer = (RgbColor *) malloc(imgSize);
    assert(ColorBuffer);

    RgbColor* pColorBuffer = ColorBuffer;

    uint8_t r = 0, g = 0, b = 0;

    if (mode == 3)
    {
    	for (size_t y = 0; y < imgHeigth; ++y)
    	{
    		for (size_t x = 0; x < imgWidth; ++x)
	    	{
	    		if (fscanf(hFile, "%hhu%hhu%hhu\n", &r, &g, &b) == 3U)
	   			{
	   				pColorBuffer[y * imgWidth + x] = (RgbColor){r, g, b};
	   			}
	    	}
    	}
    }
    else
    {
    	size_t nBytesRead = fread(pColorBuffer, imgSize, sizeof(uint8_t), hFile);
    	assert(nBytesRead == sizeof(uint8_t));
    	(void)nBytesRead;
    }
    
    pImage->Pixels = ColorBuffer;
    pImage->Mode = (mode == 3U) ? PPM_MODE_P3 : PPM_MODE_P6;

	fclose(hFile);
	return pImage;
}

void WriteToPpm(const char* filename, PpmImage* pImage, PPM_MODE mode, 
				UnaryPredicate pfnTransform)
{
	if (!filename || !*filename || !pImage || !pfnTransform)
		return;

	const char* szOpenMode = (mode == PPM_MODE_P3) ? "w" : "wb";
	FILE* hFile = fopen(filename, szOpenMode);
	
	if (!hFile)
		return;

	const char* szSignature = NULL;
	switch (mode)
	{
		case PPM_MODE_P1: szSignature = "P1"; break;
		case PPM_MODE_P3: szSignature = "P3"; break;
		case PPM_MODE_P6: szSignature = "P6"; break;
		default: break;
	}

	fprintf(hFile, "%s\n%u %u\n255\n", szSignature, pImage->Width, pImage->Heigth);

	for (uint32_t y = 0; y < pImage->Heigth; ++y)
	{
		for (uint32_t x = 0; x < pImage->Width; ++x)
		{
			RgbColor Pixel = GetPixel(x, y, pImage);
			Pixel = pfnTransform(Pixel);

			if (mode == PPM_MODE_P3)
			{
				fprintf(hFile, "%u %u %u\n", Pixel.R, Pixel.G, Pixel.B);
				assert(!ferror(hFile));
			}
			else if (mode == PPM_MODE_P1)
			{
				fprintf(hFile, "%u\n", Pixel.R);
				assert(!ferror(hFile));
			}
			else 
			{
				size_t sz = fwrite(&Pixel, sizeof(RgbColor), sizeof(uint8_t), hFile);
				assert(sz == sizeof(uint8_t));
				(void)sz;
			}
		}
	}

	fclose(hFile);
}

RgbColor ToGrayScale(RgbColor PixelColor)
{
	uint8_t r = (uint8_t)(PixelColor.R * 0.299);
	uint8_t g = (uint8_t)(PixelColor.G * 0.587);
	uint8_t b = (uint8_t)(PixelColor.B * 0.144);
    uint8_t gray = (uint8_t) floor(r + g + b + 0.5);
    return (RgbColor){gray, gray, gray};
}

RgbColor ToGrayScaleP3(RgbColor PixelColor)
{
    uint8_t gray = (uint8_t)((PixelColor.R + PixelColor.G + PixelColor.B) / 3U);
    return (RgbColor){gray, gray, gray};
}
