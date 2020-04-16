/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

// Std. Includes
#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// Audio libraries
// irrKlang for music reproduction
#include <irrKlang/irrKlang.h>
// Aubio for music processing
#include <aubio.h>

#include <imgui/imgui.h>
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders and to load models
#include <utils/shader_v1.h>
#include <utils/model_v2.h>
#include <utils/camera.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// dimensions of application's window
GLuint screenWidth = 1920, screenHeight = 1080;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// setup of Shader Programs for the 5 shaders used in the application
void SetupShaders();
// delete Shader Programs whan application ends
void DeleteShaders();
// print on console the name of current shader
void PrintCurrentShader(int shader);
// load the 6 images from disk and create an OpenGL cubemap
GLint LoadTextureCube(string path);
// draw the GUI through ImGui
void DrawGUI();
void PlayMusic(string musicPath);

// we initialize an array of booleans for each keybord key
bool keys[1024];
// we set the initial position of mouse cursor in the application window
GLfloat lastX = 400, lastY = 300;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;

// we create a camera. We pass the initial position as a paramenter to the constructor. The last boolean tells that we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_TRUE);

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;
GLboolean mouseEnabled = GL_FALSE;

// a vector for all the Shader Programs used and swapped in the application
vector<Shader> shaders;

// Uniforms to be passed to shaders
GLfloat speed = 2.0f;

// Variables
string musicPath = "../../../Music/Invincible.mp3";
GLfloat sunSize = 2.0f;
GLfloat sunDepth = 0.0f;
GLfloat gridSize = 1.0f;

// texture unit for the cube map
GLuint textureCube;

// Initialization of irrKlang for audio reproduction
irrklang::ISoundEngine* soundEngine = irrklang::createIrrKlangDevice();
	

/////////////////// MAIN function ///////////////////////
int main()
{
	if (!soundEngine)
	{
		printf("Could not startup engine\n");
		return 0; // error starting up the engine
	}
		
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 3.3 Core
    // It is possible to raise the values, in order to use functionalities of OpenGL 4.x
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	// Antialiasing samples
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	 
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	// we create the application's window
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Retrowave", monitor, NULL);
    //GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Retrowave", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);
	// MSAA Enable
	glEnable(GL_MULTISAMPLE);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //the "clear" color for the frame buffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	
	ImGui::StyleColorsDark();
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

	// we create the Shader Program used for the plane (fixed)
	Shader grid_shader("06_procedural_base.vert", "neonGrid.frag");
	shaders.push_back(grid_shader);
	Shader sun_shader("retrosun.vert", "retrosunSphere.frag");
	shaders.push_back(sun_shader);
	Shader skybox_shader("20_skybox.vert", "20_skybox.frag");
	shaders.push_back(skybox_shader);
	Shader qSun_shader("retrosun.vert", "retrosunQuad.frag");
	shaders.push_back(qSun_shader);
	
	// we load the cube map (we pass the path to the folder containing the 6 views)
    textureCube = LoadTextureCube("../../../textures/cube/Purple/");
	
    // we load the model(s) (code of Model class is in include/utils/model_v1.h)
    Model sphereModel("../../../models/sphere.obj");
	Model skyboxModel("../../../models/flippedCube.obj");
	Model gridModel("../../../models/grid500m100x100.obj");
	Model quadModel("../../../models/myPlane.obj");

    // we set projection and view matrices
    // N.B.) in this case, the camera is fixed -> we set it up outside the rendering loop
    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, -7.0f), glm::vec3(0.0f, 1.0f, 7.0f));
	
	soundEngine->play2D(musicPath.c_str(), true);
	
    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();
		// we apply FPS camera movements
        apply_camera_movements();
		// View matrix (=camera): position, view direction, camera "up" vector
        glm::mat4 view = camera.GetViewMatrix();
		
		// we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		// Draw the GUI through ImGui
		DrawGUI();
		
		// we activate the cube map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
		
		///////////////////// NEONGRID /////////////////////
		
		grid_shader.Use();
		
		glUniformMatrix4fv(glGetUniformLocation(grid_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(grid_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
		
		glm::mat4 gridModelMatrix;
		gridModelMatrix = glm::translate(gridModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
		gridModelMatrix = glm::scale(gridModelMatrix, glm::vec3(gridSize, 1.0f, gridSize));
		glUniformMatrix4fv(glGetUniformLocation(grid_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(gridModelMatrix));
		
		gridModel.Draw(grid_shader);
		
        /////////////////// SKYBOX ////////////////////////////////////////////////
		
        glDepthFunc(GL_LEQUAL);
        skybox_shader.Use();
        // we activate the cube map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
         // we pass projection and view matrices to the Shader Program of the skybox
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        // to have the background fixed during camera movements, we have to remove the translations from the view matrix
        // thus, we consider only the top-left submatrix, and we create a new 4x4 matrix
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));    // Remove any translation component of the view matrix
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint textureLocation = glGetUniformLocation(skybox_shader.Program, "tCube");
        // we assign the value to the uniform variable
        glUniform1i(textureLocation, 0);

        // we render the cube with the environment map
        skyboxModel.Draw(skybox_shader);
        // we set again the depth test to the default operation for the next frame
        glDepthFunc(GL_LESS);
		
		// Transparent objects are rendered after all opaque ones
		
		/////////// QUAD SUN ///////////////
		view = camera.GetViewMatrix();
		qSun_shader.Use();
		
		// uniforms are passed to the corresponding shader
		GLint timerLocation = glGetUniformLocation(qSun_shader.Program, "u_time");
			
		glUniform1f(timerLocation, currentFrame * speed);
		
		// we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(qSun_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(qSun_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
		
		glm::mat4 quadModelMatrix;
		
		quadModelMatrix = glm::translate(quadModelMatrix, glm::vec3(0.0f, 5.0f, sunDepth));
		quadModelMatrix = glm::rotate(quadModelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        quadModelMatrix = glm::scale(quadModelMatrix, glm::vec3(sunSize, 1.0f, sunSize));
        glUniformMatrix4fv(glGetUniformLocation(qSun_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(quadModelMatrix));
		
		quadModel.Draw(qSun_shader);
		
		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    DeleteShaders();
	
	// Delete irrKlang sound engine
	soundEngine->drop();
	
	// ImGui Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
	
    // we close and delete the created context
    glfwTerminate();
    return 0;
}


void DrawGUI()
{
	static bool fileDialog = false;
	static string fileName = "";
	
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	ImGui::Begin("GeneralUI");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	if(fileName.size() > 0)
		ImGui::Text("Current Music: %s", fileName.c_str());
	else
		ImGui::Text("Current Music: Default");

	if(ImGui::Button("Restart Music")){
		PlayMusic(musicPath);
	}
	if(ImGui::Button("Change Music")){
		soundEngine->stopAllSounds();
		fileDialog = true;
	}
	
	if(fileDialog){
		if(ImGuiFileDialog::Instance()->FileDialog("Choose Music File", "", "../../../Music", "")){
			if(ImGuiFileDialog::Instance()->IsOk == true){
				fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
				musicPath = ImGuiFileDialog::Instance()->GetFilepathName();
				
				PlayMusic(musicPath);
			}
			else{
				PlayMusic(musicPath);
			}
			fileDialog = false;
		}
	}
		
	ImGui::End();
	
	ImGui::Begin("Grid Parameters");
	ImGui::SliderFloat("GridSize", &gridSize, 0.1f, 5.0f);
	ImGui::End();
	
	ImGui::Begin("RetroSun Parameters");
	ImGui::SliderFloat("AnimationSpeed", &speed, 0.0f, 10.0f);
	ImGui::SliderFloat("SunDepth", &sunDepth, -10.0f, 10.0f);
	ImGui::SliderFloat("Size", &sunSize, 0.5f, 10.0f);
	ImGui::End();
}

void PlayMusic(string musicPath){
	soundEngine->stopAllSounds();
	soundEngine->play2D(musicPath.c_str(), true);
}

//////////////////////////////////////////
// we delete all the Shaders Programs
void DeleteShaders()
{
    for(GLuint i = 0; i < shaders.size(); i++)
        shaders[i].Delete();
}

//////////////////////////////////////////
// we load the 6 images from disk and we create an OpenGL cube map
GLint LoadTextureCube(string path)
{
    GLuint textureImage;
    int w, h;
    unsigned char* image;
    string fullname;

    glGenTextures(1, &textureImage);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);

    // we use as convention that the names of the 6 images are "posx, negx, posy, negy, posz, negz", placed at the path passed as parameter
    //POSX
    fullname = path + std::string("posx.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    //NEGX
    fullname = path + std::string("negx.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    //POSY
    fullname = path + std::string("posy.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    //NEGY
    fullname = path + std::string("negy.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    //POSZ
    fullname = path + std::string("posz.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    //NEGZ
    fullname = path + std::string("negz.jpg");
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);

    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // we set how to consider the texture coordinates outside [0,1] range
    // in this case we have a cube map, so
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureImage;

}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;
		
	if(key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS){
		mouseEnabled=!mouseEnabled;
		if(mouseEnabled)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	// we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
      // we move the camera view following the mouse cursor
      // we calculate the offset of the mouse cursor from the position in the last frame
      // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
      if(firstMouse)
      {
          lastX = xpos;
          lastY = ypos;
          firstMouse = false;
      }

      // offset of mouse cursor position
      GLfloat xoffset = xpos - lastX;
      GLfloat yoffset = lastY - ypos;

      // the new position will be the previous one for the next frame
      lastX = xpos;
      lastY = ypos;

      // we pass the offset to the Camera class instance in order to update the rendering
	  if(!mouseEnabled)
		camera.ProcessMouseMovement(xoffset, yoffset);

}