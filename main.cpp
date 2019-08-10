/*****************************************************************************
 >>> This is tmStreamer
 It streams a texture from TextureMaker onto some live render window.
 The window to be captured has to be named: capture0* [512x512, 100%].
 This should be changed in a later version.

 using
	- Boost
    - OgreSDK

TODO:
	-speed up framerate while not compromising tm-window behaviour
	-make stable
	-make more convenient

@file Main file of tmStreamer project.
@author barn
@version 20130625
******************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// INCLUDES project headers
#include "defines.h"

///////////////////////////////////////////////////////////////////////////////
//INCLUDES C/C++ standard library (and other external libraries)
#include <iostream>
#include <string>
#include <assert.h>

#include <Windows.h>
#include <GdiPlus.h>
#include <GdiPlusPixelFormats.h>

#include <OGRE/Ogre.h>
#include <OGRE/OgreHardwarePixelBuffer.h>

#include "../_Lib/barn_common.h"
#include "../_Lib/barn_util_Ogre.h"

///////////////////////////////////////////////////////////////////////////////
// DEFINES and MACROS

///////////////////////////////////////////////////////////////////////////////
// NAMESPACE, CONSTANTS and TYPE DECLARATIONS/IMPLEMENTATIONS
using namespace std;
using namespace Gdiplus;

std::pair<float, float> create_dynamic_texture(
	HWND hwnd,
	const std::string& texture_name,
	int crop_left=0,
	int crop_right=0,
	int crop_top=0,
	int crop_bottom=0);

/// main function
int main(int argc, const char* argv[]) {
	cout << "*****************************" << endl
		 << "*** Welcome to tmStreamer ***" << endl
		 << "*****************************" << endl
		 << endl;

	std::string window_title;

	// find window to capture
	if(argc < 2) {
		std::string in_string;

		cout << "Usage: tmStreamer.exe <frame TM-Subwindow name>" << endl
			 << "e.g.: tmStreamer.exe \"capture0* [512x512, 100%]\"" << endl << endl
			 << "please enter a non-maximized (!) Texture Maker texture window to find\n: ";
		std::getline( std::cin, window_title);

    } else {
		window_title = argv[1];
	}

	//// +++ init ogre +++

	Ogre::Root* ogre = new Ogre::Root( OGRE_PLUGIN_FILE, OGRE_CONFIG_FILE, LOG_FILE);

	if(!ogre->restoreConfig() && !ogre->showConfigDialog()) {
		DESTROY(ogre);
		EXIT(-2);
	}

	// necessary to init the window before initialization of the resources

	ogre->initialise( false);
	auto ogre_window = boilerplate_ogre::create_window_from_config_file(
		ogre,
		TM_STREAMER_CONFIG_FILE,
		TM_STREAMER_WINDOW_PARAMETER_SECTION
		);
	//// === init ogre complete ===

	//// +++ init gdi+ +++
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	//// === init gdi+ ===

	//// +++ get window handle +++

	auto hwnd_tm         = FindWindow(NULL, "Texture Maker Enterprise v3.0.3");
	auto hwnd_mdi_client = FindWindowEx( hwnd_tm, NULL, "MDIClient", NULL);
	auto hwnd            = FindWindowEx( hwnd_mdi_client, NULL, NULL, window_title.c_str());

	assert( hwnd_tm != 0);
	assert( hwnd_mdi_client != 0);
	assert( hwnd != 0);

	if(hwnd == 0) {
		std::ostringstream oss;
		oss << "Window \"" << window_title << "\" not found.\n"
			<< "Shutting down.";
		Ogre::LogManager::getSingleton().getLog( LOG_FILE)->logMessage( oss.str());

		DESTROY(ogre);
		EXIT(-3);
	}
	//// === get window handle ===

	//// +++ create scene +++
	Ogre::SceneManager* scene_manager = ogre->createSceneManager( Ogre::ST_GENERIC, "SceneManager");
	Ogre::Camera* camera = scene_manager->createCamera( "Cam");
	Ogre::Viewport* viewport = ogre_window->addViewport( camera);
	//// === create scene ===
	viewport->setBackgroundColour( Ogre::ColourValue( 1,0,0,1));
	//// +++ create background image+++

	// Create background rectangle covering the whole screen
	Ogre::Rectangle2D* rect = new Ogre::Rectangle2D(true);
	rect->setCorners(-1.0, 1.0, 1.0, -1.0);

	// Render the background before everything else
	rect->setRenderQueueGroup(Ogre::RENDER_QUEUE_BACKGROUND);

	// Use infinite AAB to always stay visible
	Ogre::AxisAlignedBox aabInf;
	aabInf.setInfinite();
	rect->setBoundingBox(aabInf);

	// Attach background to the scene
	Ogre::SceneNode* node = scene_manager->getRootSceneNode()->createChildSceneNode("Background");
	node->attachObject(rect);

	// Create background material
	Ogre::TextureManager* texture_manager = Ogre::TextureManager::getSingletonPtr();

	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("Background", "General");
	auto texture_unit_state = material->getTechnique(0)->getPass(0)->createTextureUnitState("DynamicTexture");
	material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

	create_dynamic_texture( hwnd, "DynamicTexture", 2, 2, 22, 16);
	rect->setMaterial("Background");
	//// === create background image===

	//// render loop
	while(true) {
		std::pair<float,float> dims = create_dynamic_texture( hwnd, "DynamicTexture", 2, 2, 22, 16);

		texture_unit_state->setTexture( texture_manager->getByName("DynamicTexture"));
		rect->setUVs(
			Ogre::Vector2(0,          0),
			Ogre::Vector2(0,          dims.second),
			Ogre::Vector2(dims.first, 0),
			Ogre::Vector2(dims.first, dims.second)
			);


		if (ogre_window->isActive())
		   ogre->renderOneFrame();
		else if (ogre_window->isVisible())
		   ogre_window->setActive(true);

		Ogre::WindowEventUtilities::messagePump();

		Sleep(42);
	}

	//// *** shutdown everything ****

	DESTROY(rect);
	DESTROY(scene_manager);
	DESTROY(ogre);

	EXIT(0);
} // END main

/**
 * Grabs the image of some hwnd and renders it onto an ogre texture.
 * Since some GPUs cannot handle non-power-of-two textures, the ogre texture will have
 * power-of-two dimensions. Where the upper left part of the texture contains the
 * captured image, the rest of the image will stay black.
 * @param hwnd The window handle. Should be valid ;)
 * @param crop_left The cropping value for the left side in pixels.
 * @param crop_right The cropping value for the right side in pixels.
 * @param crop_top The cropping value for the top side in pixels.
 * @param crop_bottom The cropping value for the bottom side in pixels.
 * @return The ratio of the captured bitmap sizes and the produced ogre texture size as a pair of floats,
 * where the first value is the ratio for width and the second value the ratio for the height.
 */
std::pair<float, float> create_dynamic_texture(
	HWND hwnd,
	const std::string& texture_name,
	int crop_left,
	int crop_right,
	int crop_top,
	int crop_bottom) {

	RECT rc;
	GetClientRect(hwnd, &rc);

	Rect rc_crop(
		crop_left,
		crop_top,
		rc.right - rc.left - crop_left - crop_right,
		rc.bottom - rc.top - crop_top - crop_bottom
		);

	//create device contexts and hbitmaps
	HDC hdc_screen = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(hdc_screen);
	HBITMAP hbm = CreateCompatibleBitmap(
		hdc_screen,
		rc.right - rc.left - crop_right,
		rc.bottom - rc.top - crop_bottom
		);

	// connects the bitmap with the devicecontext
	// and prints image to connected devicecontext
	SelectObject(hdc, hbm);
	PrintWindow(hwnd, hdc, PW_CLIENTONLY);

	//release unused stuff #1
	DeleteDC(hdc);
	ReleaseDC(NULL, hdc_screen);

	// create bitmap and bitmapdata
	Bitmap bitmap(hbm, NULL);
	BitmapData bitmap_data;

	//release unused stuff #2
	DeleteObject(hbm);

	// if a texture with this name already exists... delete it
	auto res_ptr = Ogre::TextureManager::getSingleton().getByName(texture_name);
	if( res_ptr.getPointer())
		Ogre::TextureManager::getSingleton().remove( res_ptr);

	// lock bitmap bits
	Gdiplus::Status status =
		bitmap.LockBits( &rc_crop, ImageLockMode::ImageLockModeRead, PixelFormat24bppRGB, &bitmap_data);
	if( status != Status::Ok) {

		std::ostringstream oss;
		oss <<  "Error locking bits of given hwnd " << hwnd << ": Gdiplus::Status status= " << status << "\n";
		Ogre::LogManager::getSingleton().getLog( LOG_FILE)->logMessage( oss.str());
	}

	const unsigned int texture_width  = nearestPowTwo( bitmap_data.Width);
	const unsigned int texture_height = nearestPowTwo( bitmap_data.Height);

	// Create the texture
	Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(
		texture_name,									// name
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D,								// type
		texture_width,										// width
		texture_height,										// height
		0,												// number of mipmaps
		Ogre::PF_BYTE_RGB,								// pixel format
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);		// usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
														// textures updated very often (e.g. each frame)

	// Get the pixel buffer
	// Lock the pixel buffer and get a pixel box
	Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD); // for best performance use HBL_DISCARD!
	const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

	// Fill in pixel data.
	for (size_t i = 0; i < texture_height ; ++i)
		for(size_t j = 0; j < texture_width*3 ; j+=3) {

			if( bitmap_data.Width*3<=j || bitmap_data.Height<=i) {
				pDest += 4;
				continue;
			}

			*pDest++ = ((unsigned char*)bitmap_data.Scan0)[i*bitmap_data.Stride+j];		// R
			*pDest++ = ((unsigned char*)bitmap_data.Scan0)[i*bitmap_data.Stride+j+1];	// G
			*pDest++ = ((unsigned char*)bitmap_data.Scan0)[i*bitmap_data.Stride+j+2];	// B
			++pDest;																	// A
		}

	// Unlock the bitmap bits and pixel buffer
	bitmap.UnlockBits(&bitmap_data);
	pixelBuffer->unlock();

	return std::pair<float, float>((float)bitmap_data.Width/texture_width, (float)bitmap_data.Height/texture_height);
}
