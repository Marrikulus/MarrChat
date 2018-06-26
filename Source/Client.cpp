#include <csignal>
#include <iostream>
#include <sstream>


#include <GL/glew.h>
#define GL3_PROTOTYPES 1
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Common.h"



#define MAX_TEXT_LENGTH 256

static const int SCREEN_WIDTH  = 960;
static const int SCREEN_HEIGHT = 540;
static SDL_Window *window = NULL;
static SDL_GLContext context;
static RakNet::Packet *packet = NULL;

static char text[MAX_TEXT_LENGTH];


static struct
{
	GLuint ProgramId;
	GLuint VertexShaderId;
	GLuint FragmentShaderId;

	GLuint VertexBufferHandle;
	GLuint VertexArrayHandle;
	GLuint ElementBufferObject;
} Opengl;


const u32 points = 4;
const u32 floatsPerPoint = 3;
const u32 floatsPerColor = 4;

const GLfloat diamond[points][floatsPerPoint] = {
	{ -0.5,  0.5,  0.5 }, // Top left
	{  0.5,  0.5,  0.5 }, // Top right
	{  0.5, -0.5,  0.5 }, // Bottom right
	{ -0.5, -0.5,  0.5 }, // Bottom left
};

const GLfloat colors[points][floatsPerColor] = {
	{ 0.0, 1.0, 0.0, 1.0 }, // Top left
	{ 1.0, 1.0, 0.0, 1.0  }, // Top right
	{ 1.0, 0.0, 0.0, 1.0  }, // Bottom right
	{ 0.0, 0.0, 1.0, 1.0  }, // Bottom left
};


void processNetwork(RakNet::RakPeerInterface *peer)
{
	for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
	{
		switch (packet->data[0])
		{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			{
				std::cout << "Another client has disconnected." << std::endl;
			}break;
			case ID_REMOTE_CONNECTION_LOST:
			{
				std::cout << "Another client has lost the connection." << std::endl;
			}break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
			{
				std::cout << "Another client has connected." << std::endl;
			}break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				std::cout << "Our connection request has been accepted." << std::endl;


				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)ID_MARR_MESSAGE);
				bsOut.Write("Hello, I'm Client");

				peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
			}break;
			case ID_NEW_INCOMING_CONNECTION:
			{
				std::cout << "A connection is incoming." << std::endl;
			}break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
			{
				std::cout << "The server is full." << std::endl;
			}break;
			case ID_DISCONNECTION_NOTIFICATION:
			{
				std::cout << "We have been disconnected." << std::endl;
			}break;
			case ID_CONNECTION_LOST:
			{
				std::cout << "Connection lost." << std::endl;
			}break;
			case ID_MARR_MESSAGE:
			{
				RakNet::RakString rs;
				RakNet::BitStream bsIn(packet->data,packet->length,false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.Read(rs);
				std::cout << rs.C_String() << std::endl;
			}break;
			default:
			{
				std::cout << "Message with identifier " << packet->data[0] << " has arrived." << std::endl;
			} break;
		}
	}
}

//u32 wrap = 200;
//TTF_RenderText_Blended_Wrapped(font, text, color, wrap);

void drawText(char *text, float x, float y)
{
	glEnable(GL_TEXTURE_2D);
	TTF_Font * font = TTF_OpenFont("FreeSans.ttf", 30);
	if (font == NULL)
	{
		std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
		return;
	}

	GLuint Handle = 0;

	SDL_Color color = { 0, 0, 0 };
	auto message = TTF_RenderText_Blended(font, text, color);

	if (message)
	{
		glBindVertexArray(Opengl.VertexArrayHandle);
		float w = message->w;
		float h = message->h;

		float vertices[6][4] = {
			{ x, 	y+h, 	0.0f, 0.0f },
			{ x,  	y, 		0.0f, 1.0f },
			{ x+w,	y, 		1.0f, 1.0f },

			{ x, 	y+h, 	0.0f, 0.0f },
			{ x+w,	y, 		1.0f, 1.0f },
			{ x+w, 	y+h, 	1.0f, 0.0f },
		};

		glGenTextures(1, &Handle);
		glBindTexture(GL_TEXTURE_2D, Handle);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, message->w, message->h, 0,
						GL_BGRA, GL_UNSIGNED_BYTE, message->pixels);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindBuffer(GL_ARRAY_BUFFER, Opengl.VertexBufferHandle);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		glDrawArrays(GL_TRIANGLES, 0,  6);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &Handle);
		glBindVertexArray(0);
	}

	SDL_FreeSurface(message);

	TTF_CloseFont(font);
	glDisable(GL_TEXTURE_2D);
}

int main(int argc, char const *argv[])
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;
	if (TTF_Init() == -1)
		return 1;

	u32 startclock = 0;
	u32 deltaclock = 0;
	u32 currentFPS = 0;

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int flags = SDL_WINDOW_OPENGL;// | SDL_WINDOW_SHOWN;

	window = SDL_CreateWindow("Hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
								  SCREEN_WIDTH, SCREEN_HEIGHT, flags);
	if (!window)
		return 1;

	context = SDL_GL_CreateContext(window);

	#ifndef __APPLE__
	glewExperimental = GL_TRUE;
	glewInit();
	#endif

	SDL_GL_SetSwapInterval(1);
	glViewport(0, 0, 800, 600);
	glClearColor ( 0.5, 0.5, 0.5, 1.0 );
	SDL_Event event;
	SDL_Rect position;

	GLchar *VertexCode[] =
	{
		R"FOO(
		#version 330 core
		#define V4 vec4
		#define V3 vec3
		#define V2 vec2

		#define v4 vec4
		#define v3 vec3
		#define v2 vec2

		layout (location = 0) in v4 vertex;

		uniform mat4 projection;
		out v2 textureCoord;

		void main()
		{
			gl_Position = projection * v4(vertex.xy, 0.0, 1.0);
			textureCoord = vertex.zw;
		}
		)FOO"
	};
	Opengl.VertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Opengl.VertexShaderId, 1, VertexCode, 0);
	glCompileShader(Opengl.VertexShaderId);

	GLchar *FragmentCode[] =
	{
		R"FOO(
		#version 330 core
		#define V4 vec4
		#define V3 vec3
		#define V2 vec2

		#define v4 vec4
		#define v3 vec3
		#define v2 vec2

		out v4 FragColor;
		in v2 textureCoord;

		uniform sampler2D ourTexture;

		void main()
		{
			FragColor = texture(ourTexture, textureCoord);
		}
		)FOO",
	};
	Opengl.FragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Opengl.FragmentShaderId, 1, FragmentCode, 0);
	glCompileShader(Opengl.FragmentShaderId);

	Opengl.ProgramId = glCreateProgram();
	glAttachShader(Opengl.ProgramId, Opengl.VertexShaderId);
	glAttachShader(Opengl.ProgramId, Opengl.FragmentShaderId);
	glLinkProgram(Opengl.ProgramId);

	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd;

	std::stringstream ss;

	text[0] = 0;
	bool quit = false;
	SDL_StartTextInput();

	peer->Startup(1, &sd, 1);
	peer->Connect("127.0.0.1", SERVER_PORT, 0, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &Opengl.VertexArrayHandle);
	glGenBuffers(1, &Opengl.VertexBufferHandle);

	glBindVertexArray(Opengl.VertexArrayHandle);
	glBindBuffer(GL_ARRAY_BUFFER, Opengl.VertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 , (void*)0 );
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
	glUseProgram(Opengl.ProgramId);
	glUniformMatrix4fv(glGetUniformLocation(Opengl.ProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	while (!quit)
	{
		startclock = SDL_GetTicks();

		processNetwork(peer);

		while (SDL_PollEvent(&event))
		{
			switch( event.type )
			{
				case SDL_QUIT:{
					quit = true;
				} break;

				case SDL_KEYDOWN:{
					switch( event.key.keysym.sym )
					{
						case SDLK_RETURN:
						{
							int textlen = SDL_strlen(text);
							std::cout << "Length: " << textlen << " text: '" << text << "'" << std::endl;

							if (peer->GetSystemAddressFromIndex(0)!=RakNet::UNASSIGNED_SYSTEM_ADDRESS)
							{
								std::cout << peer->GetSystemAddressFromIndex(0).ToString() << std::endl;
								RakNet::BitStream bsOut;
								bsOut.Write((RakNet::MessageID)ID_MARR_MESSAGE);
								bsOut.Write(text);
								peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0, peer->GetSystemAddressFromIndex(0),false);
							}

							text[0]=0x00;
						}break;

						case SDLK_ESCAPE:{
							quit = true;
						} break;

						case SDLK_1:
						{
							std::cout << "Number: " << event.key.keysym.sym << std::endl;
						} break;
					}
				} break;
				case SDL_TEXTINPUT:
				{
					std::cerr << "Keyboard: text input \"" << event.text.text << "\"" << std::endl;
					if (SDL_strlen(text) + SDL_strlen(event.text.text) < sizeof(text))
						SDL_strlcat(text, event.text.text, sizeof(text));
				} break;
			}
		}

		glClear ( GL_COLOR_BUFFER_BIT );


		//glBindVertexArray(Opengl.VertexArrayHandle);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		//position.x = SCREEN_WIDTH / 3;
		//position.y = SCREEN_HEIGHT / 2;

		//drawText("Testdfdf", 50, 50);
		drawText(text, 50.0f, 300.0f);


		SDL_GL_SwapWindow(window);

		deltaclock = SDL_GetTicks() - startclock;
		startclock = SDL_GetTicks();

		if ( deltaclock != 0 )
		{
			currentFPS = 1000 / deltaclock;
			ss.str("");
			ss << "Hi - " << currentFPS << " fps";
			SDL_SetWindowTitle(window, ss.str().c_str());
		}
	}



	glUseProgram(0);
	glDetachShader(Opengl.ProgramId, Opengl.VertexShaderId);
	glDetachShader(Opengl.ProgramId, Opengl.FragmentShaderId);
	glDeleteProgram(Opengl.ProgramId);
	glDeleteShader(Opengl.VertexShaderId);
	glDeleteShader(Opengl.FragmentShaderId);


	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &Opengl.VertexBufferHandle);
	glDeleteBuffers(1, &Opengl.ElementBufferObject);
	glDeleteVertexArrays(1, &Opengl.VertexArrayHandle);

	SDL_StopTextInput();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();

	RakNet::RakPeerInterface::DestroyInstance(peer);
	return 0;
}