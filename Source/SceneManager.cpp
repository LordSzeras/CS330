///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
	// clear the collection of defined materials
	m_objectMaterials.clear();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/speaker_body.jpg",
		"speaker_body");

	bReturn = CreateGLTexture(
		"textures/speaker_mesh.jpg",
		"speaker_mesh");

	bReturn = CreateGLTexture(
		"textures/speaker_screws.jpg",
		"speaker_screws");

	bReturn = CreateGLTexture(
		"textures/speaker_ring.jpg",
		"speaker_ring");

	//bReturn = CreateGLTexture(
	//	"textures/speaker_light",
	//	"speaker_light");

	bReturn = CreateGLTexture(
		"textures/desk_top.jpg",
		"desk_top");

	bReturn = CreateGLTexture(
		"textures/plastic.jpg",
		"plastic");

	bReturn = CreateGLTexture(
		"textures/keyboard.jpg",
		"keyboard");

	bReturn = CreateGLTexture(
		"textures/screen.jpg",
		"screen");

	bReturn = CreateGLTexture(
		"textures/keys.jpg",
		"keys");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL blackScrews;
	blackScrews.diffuseColor = glm::vec3(0.02f, 0.04f, 0.04f);
	blackScrews.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	blackScrews.shininess = 35.0;
	blackScrews.tag = "black screws";

	m_objectMaterials.push_back(blackScrews);

	OBJECT_MATERIAL porcelainMaterial;
	porcelainMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	porcelainMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	porcelainMaterial.shininess = 5.0;
	porcelainMaterial.tag = "porcelain";

	m_objectMaterials.push_back(porcelainMaterial);

	OBJECT_MATERIAL blackPlastic;
	blackPlastic.diffuseColor = glm::vec3(0.05f, 0.05f, 0.05f);
	blackPlastic.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	blackPlastic.shininess = 10.0;
	blackPlastic.tag = "black plastic";

	m_objectMaterials.push_back(blackPlastic);

}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// overhead room light coming into scene
	//m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -3.5f, -5.0f);
	//m_pShaderManager->setVec3Value("directionalLight.ambient", 0.65f, 0.65f, 0.65f);
	//m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	//m_pShaderManager->setVec3Value("directionalLight.specular", 0.1f, 0.1f, 0.1f);
	//m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// futre point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 20.0f, 20.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.86f, 0.85f, 0.88f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// future point light 2
	//m_pShaderManager->setVec3Value("pointLights[1].position", 100.0f, 100.0f, 100.0f);
	//m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.55f, 0.55f, 0.55f);
	//m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	//m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	//m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	


}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures for objects
	DefineObjectMaterials();
	LoadSceneTextures();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(35.0f, 1.0f, 13.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetTextureUVScale(4.00f, 10.0f);
	SetShaderTexture("desk_top");
	SetShaderMaterial("porcelain");
	

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//Left Speaker

	/******************************************************************/
	// Left Speaker Box
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 5.5f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 2.7f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_body");
	SetShaderMaterial("black plastic");
	

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker Light Bar
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, .05f, .05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.66f, 0.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1);
	//SetShaderTexture("speaker_light");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker Ring(torus)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.55f, 1.55f, .3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 2.8f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.3, .3, .3, 1);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_ring");
	SetShaderMaterial("black plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker upper left screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.9f, 3.65f, -4.77f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker upper right screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.3f, 3.65f, -5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker lower left screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.9f, 1.95f, -4.77f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker lower right screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.3f, 1.95f, -5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Left Speaker mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, .07f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 2.8f, -5.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.15, 0.15, 0.15, 1);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_mesh");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Right Speaker

	/******************************************************************/
	// Right Speaker Box
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 5.5f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 2.7f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_body");
	SetShaderMaterial("black plastic");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker Light Bar
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, .05f, .05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.66f, 0.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1);
	//SetShaderTexture("speaker_light");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker Ring(torus)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.55f, 1.55f, .3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.6f, 2.8f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.3, .3, .3, 1);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_ring");
	SetShaderMaterial("black plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker upper left screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.9f, 3.65f, -4.77f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker upper right screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.3f, 3.65f, -5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker lower left screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.9f, 1.95f, -4.77f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker lower right screw
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.17f, .08f, .17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.3f, 1.95f, -5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("speaker_screws");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Right Speaker mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, .07f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.6f, 2.8f, -5.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.15, 0.15, 0.15, 1);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_mesh");
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// Computer Tower

	/******************************************************************/
	//Tower Box
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 13.0f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(23.0f, 6.5f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("keyboard");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Tower Glass
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 0.0f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.9f, 6.5f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.15, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Primary monitor

	/******************************************************************/
	//Monitor mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(24.0f, 14.0f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 14.0f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.35, 0.35, 0.35, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Monitor Screen mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.5f, 6.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 14.0f, -3.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.55, 0.55, 0.55, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("screen");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Mouse Pad

	/******************************************************************/
	//Mouse Pad mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(34.0f, 0.2f, 13.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 0.1f, 5.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05, 0.05, 0.05, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	//SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// Secondary monitor

	/******************************************************************/
	//Monitor mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(11.0f, 19.0f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-22.0, 12.0f, -2.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.35, 0.35, 0.35, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Monitor screen mesh
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 5.0f, 8.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-21.86f, 12.0f, -1.86f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.55, 0.55, 0.55, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("screen");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Primary monitor arm

	/******************************************************************/
	//Primary monitor arm base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 0.5f, -10.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm lower holder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.90f, 5.4f, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 1.0f, -11.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm back plate
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.4f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 13.5f, -4.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm back plate knuckle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0, 13.5f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm back plate knuckle top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.70f, 0.5f, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -70.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.2, 14.0f, -5.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm back plate knuckle bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.70f, 0.5f, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -70.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.2, 13.0f, -5.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.8f, 1.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -4.0f;
	YrotationDegrees = 18.0f;
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5f, 12.2f, -7.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm lower
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.8f, 1.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -4.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5, 7.5f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

		/******************************************************************/
	//Primary monitor arm elbow
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 3.6f, 1.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 7.8f, -8.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Primary monitor arm back plate holder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.2f, 0.80f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.6, 12.5f, -6.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderMaterial("black screws");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// Secondary Monitor arm and base

	/******************************************************************/
	//Secondary monitor arm back plate connector
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 4.0f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 205.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-22.7f, 14.2f, -3.0f);
	//positionXYZ = glm::vec3(0.0f, 6.5f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.15, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	/****************************************************************/

	/******************************************************************/
	//Secondary monitor arm back plate swivel
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.85f, 3.9f, 0.85f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 205.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-24.9f, 14.2f, -3.1f);
	//positionXYZ = glm::vec3(0.0f, 6.5f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05, 0.05, 0.05, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_ring");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Secondary monitor arm
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 15.0f, 1.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -4.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-23.45f, 8.3f, -4.55f);
	//positionXYZ = glm::vec3(0.0f, 6.5f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.65, 0.15, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Secondary monitor arm base swivel
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 0.7f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-23.45f, 0.3f, -4.55f);
	//positionXYZ = glm::vec3(0.0f, 6.5f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.65, 0.65, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_ring");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/******************************************************************/
	//Secondary monitor arm base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.5f, 0.5f, 8.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-23.45f, 0.25f, -4.55f);
	//positionXYZ = glm::vec3(0.0f, 6.5f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25, 0.25, 0.25, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// Keyboard

	/******************************************************************/
	//Keyboard
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 0.7f, 4.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 8.5f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.9f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.65, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("keyboard");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Keyboard keys
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.9f, 0.7f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 8.5f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.28f, 4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.65, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("keys");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/******************************************************************/
	//Keyboard back left stand
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.9f, 1.1f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 15.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.6f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.25, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Keyboard back right stand
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.9f, 1.1f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 15.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.6f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.25, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/******************************************************************/
	//Keyboard wrist rest
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.69f, 9.90f, 2.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.4f, 7.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.15, 0.25, 0.15, 1);
	//SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("speaker_mesh");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	/****************************************************************/


}
