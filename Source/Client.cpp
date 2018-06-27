#include <csignal>
#include <iostream>
#include <sstream>
#include <vector>


#include <GL/glew.h>
#define GL3_PROTOTYPES 1
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Common.h"

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define MAX_TEXT_LENGTH 256

static const int 	SCREEN_WIDTH  = 800;
static const int 	SCREEN_HEIGHT = 600;
static const float 	SCREEN_WIDTH_F  = 800.0f;
static const float 	SCREEN_HEIGHT_F = 600.0f;
static SDL_Window *window = NULL;
static SDL_GLContext context;
static SDL_Event event;
static RakNet::Packet *packet = NULL;
static bool quit = false;

static char text[MAX_TEXT_LENGTH];


static struct OpenGL
{
	GLuint ProgramId;
	GLuint VertexShaderId;
	GLuint FragmentShaderId;

	GLuint VertexBufferHandle;
	GLuint VertexArrayHandle;
} Opengl;


struct Message
{
	char text[255];
	char name[24];
};


void processNetwork(RakNet::RakPeerInterface *peer);
float drawText(char *text, float x, float y);
void processEvents(std::vector<Message> *messages, RakNet::RakPeerInterface *peer);
void TextShader(OpenGL*);

//u32 wrap = 200;
//TTF_RenderText_Blended_Wrapped(font, text, color, wrap);



int main(int argc, char const *argv[])
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;

	if (TTF_Init() == -1)
	{
		SDL_Quit();
		return 1;
	}

	u32 startclock = 0;
	u32 deltaclock = 0;
	u32 currentFPS = 0;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int flags = SDL_WINDOW_OPENGL;// | SDL_WINDOW_SHOWN;

	window = SDL_CreateWindow("Hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
								  SCREEN_WIDTH, SCREEN_HEIGHT, flags);
	if (!window)
	{
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	context = SDL_GL_CreateContext(window);

	#ifndef __APPLE__
	glewExperimental = GL_TRUE;
	glewInit();
	#endif

	SDL_GL_SetSwapInterval(1);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClearColor ( 0.5, 0.5, 0.5, 1.0 );


	std::stringstream ss;
	text[0] = 0;

	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd;

	peer->Startup(1, &sd, 1);
	peer->Connect("127.0.0.1", SERVER_PORT, 0, 0);

	std::vector<Message> messages = {};

	TextShader(&Opengl);

	//GLint num_uniforms;
	//glGetProgramiv(Opengl.ProgramId, GL_ACTIVE_UNIFORMS, &num_uniforms);

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

	glm::mat4 projection = glm::ortho(0.0f, SCREEN_WIDTH_F, 0.0f, SCREEN_HEIGHT_F);
	glUseProgram(Opengl.ProgramId);
	glUniformMatrix4fv(glGetUniformLocation(Opengl.ProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	SDL_StartTextInput();

	while (!quit)
	{
		startclock = SDL_GetTicks();
		processNetwork(peer);
		processEvents(&messages, peer);

		glClear( GL_COLOR_BUFFER_BIT );

		float nameX = 10.0f;
		float textX = 100.0f;
		float y = SCREEN_HEIGHT_F - 30.0f;

		for (auto message : messages)
		{
			drawText(message.name, nameX, y);
			y -= drawText(message.text, textX, y);
		}

		drawText(text, 30.0f, 30.0f);

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


	SDL_StopTextInput();

	glUseProgram(0);
	glDetachShader(Opengl.ProgramId, Opengl.VertexShaderId);
	glDetachShader(Opengl.ProgramId, Opengl.FragmentShaderId);
	glDeleteProgram(Opengl.ProgramId);
	glDeleteShader(Opengl.VertexShaderId);
	glDeleteShader(Opengl.FragmentShaderId);


	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &Opengl.VertexBufferHandle);
	glDeleteVertexArrays(1, &Opengl.VertexArrayHandle);

	RakNet::RakPeerInterface::DestroyInstance(peer);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	TTF_Quit();
	SDL_Quit();

	return 0;
}

void processEvents(std::vector<Message> *messages, RakNet::RakPeerInterface *peer)
{
	while( SDL_PollEvent(&event) )
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

						if (textlen > 0)
						{
							//char *newMessage = new char[textlen+1];
							//for (int i = 0; i < textlen; ++i)
							//{
							//	newMessage[i] = text[i];
							//}
							//newMessage[textlen] = 0;

							Message msg = {};
							strncpy(msg.name, "Ingimar", 8);
							strncpy(msg.text, text, MIN(sizeof(msg.text), textlen));
							messages->push_back(msg);

							if (peer->GetSystemAddressFromIndex(0) != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
							{
								std::cout << peer->GetSystemAddressFromIndex(0).ToString() << std::endl;
								RakNet::BitStream bsOut;
								bsOut.Write((RakNet::MessageID)ID_MARR_MESSAGE);
								bsOut.Write(text);
								peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0, peer->GetSystemAddressFromIndex(0),false);
							}

							text[0] = 0x00;
						}
					}break;

					case SDLK_BACKSPACE:
					{
						int textlen = SDL_strlen(text);
						if (textlen)
							text[textlen-1] = 0;
					} break;

					case SDLK_ESCAPE:{
						quit = true;
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
}

void TextShader(OpenGL * op)
{
	GLchar *VertexCode[] =
	{
		R"FOO(
		#version 330 core

		#define v4 vec4
		#define v3 vec3
		#define v2 vec2

		layout (location = 0) in v4 vertex;

		uniform bool isText;
		uniform mat4 projection;
		out v2 textureCoord;

		void main()
		{
			gl_Position = projection * v4(vertex.xy, 0.0, 1.0);
			textureCoord = vertex.zw;
		}
		)FOO"
	};

	op->VertexShaderId = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(op->VertexShaderId, 1, VertexCode, 0);
	glCompileShader(op->VertexShaderId);

	GLchar *FragmentCode[] =
	{
		R"FOO(
		#version 330 core

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
	op->FragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(op->FragmentShaderId, 1, FragmentCode, 0);
	glCompileShader(op->FragmentShaderId);

	op->ProgramId = glCreateProgram();
	glAttachShader(op->ProgramId, op->VertexShaderId);
	glAttachShader(op->ProgramId, op->FragmentShaderId);
	glLinkProgram(op->ProgramId);
}

float drawSquare(float x, float y, float w, float h)
{
	glBindVertexArray(Opengl.VertexArrayHandle);

	float vertices[6][4] = {
		{ x, 	y, 		0.0f, 0.0f },
		{ x,  	y-h, 	0.0f, 1.0f },
		{ x+w,	y-h, 	1.0f, 1.0f },

		{ x, 	y, 		0.0f, 0.0f },
		{ x+w,	y-h, 	1.0f, 1.0f },
		{ x+w, 	y, 		1.0f, 0.0f },
	};

	glGenTextures(1, &Handle);
	glBindTexture(GL_TEXTURE_2D, Handle);


	glUniformMatrix4fv(glGetUniformLocation(Opengl.ProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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

float drawText(char *text, float x, float y)
{
	glEnable(GL_TEXTURE_2D);
	TTF_Font * font = TTF_OpenFont("FreeSans.ttf", 20);
	if (font == NULL)
	{
		std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
		return 0.0f;
	}

	GLuint Handle = 0;
	float w, h = 0.0f;

	SDL_Color color = { 0, 0, 0 };
	auto message = TTF_RenderText_Blended(font, text, color);

	if (message)
	{
		glBindVertexArray(Opengl.VertexArrayHandle);
		w = message->w;
		h = message->h;

		float vertices[6][4] = {
			{ x, 	y, 		0.0f, 0.0f },
			{ x,  	y-h, 	0.0f, 1.0f },
			{ x+w,	y-h, 	1.0f, 1.0f },

			{ x, 	y, 		0.0f, 0.0f },
			{ x+w,	y-h, 	1.0f, 1.0f },
			{ x+w, 	y, 		1.0f, 0.0f },
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

	return h;
}

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
