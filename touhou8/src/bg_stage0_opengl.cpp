#include "SDL_opengl.h"
#include "GL/glu.h"

#include "Game.h"

namespace th {

	static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = nullptr;

	void stage0_draw_background(float delta) {
		auto& assets = Assets::GetInstance();
		auto& game = Game::GetInstance();
		auto& stage = Stage::GetInstance();
		SDL_Renderer* renderer = game.renderer;

		if (!glUseProgramObjectARB) {
			glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glUseProgramObjectARB");
		}

		SDL_Texture* texture = assets.FindTexture("MistyLakeTexture");
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);

		SDL_Rect view;
		float xscale;
		float yscale;
		SDL_RenderGetViewport(renderer, &view);
		SDL_RenderGetScale(renderer, &xscale, &yscale);

		SDL_RenderFlush(renderer);

		{
			SDL_GL_BindTexture(texture, 0, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			SDL_GL_UnbindTexture(texture);

			glViewport( (view.x * xscale),
					    (view.y * yscale),
					    (view.w * xscale),
					    (view.h * yscale) );

			float r = 2.0f;

			float time = stage.time;

			float camera_x = 128.0f;
			float camera_y = 256.0f * r + 200.0f - fmodf(time / 10.0f, 256.0f);
			float camera_z = 90.0f;

			float target_x = camera_x;
			float target_y = camera_y - 100.0f;
			float target_z = 0.0f;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(60, (float)PLAY_AREA_W / (float)PLAY_AREA_H, 1, 1000);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(camera_x, camera_y, camera_z, target_x, target_y, target_z, 0, 0, 1);

			glUseProgramObjectARB(0);

			float fogColor[4] = {1, 1, 1, 1};
			glFogfv(GL_FOG_COLOR, fogColor);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glHint(GL_FOG_HINT, GL_NICEST);
			glFogf(GL_FOG_START, 100);
			glFogf(GL_FOG_END, 400);
			glEnable(GL_FOG);

			GLboolean blend_was_enabled = glIsEnabled(GL_BLEND);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			SDL_GL_BindTexture(texture, 0, 0);
			{
				glBegin(GL_QUADS);
				{
					float off = fmodf(time / 10.0f, 256.0f);

					glColor4f(1, 1, 1, 1);
					glTexCoord2f(-r,     -r    ); glVertex3f( off + 256.0f * -r,       256.0f * -r,       0);
					glTexCoord2f(r+1.0f, -r    ); glVertex3f( off + 256.0f * (r+1.0f), 256.0f * -r,       0);
					glTexCoord2f(r+1.0f, r+1.0f); glVertex3f( off + 256.0f * (r+1.0f), 256.0f * (r+1.0f), 0);
					glTexCoord2f(-r,     r+1.0f); glVertex3f( off + 256.0f * -r,       256.0f * (r+1.0f), 0);

					glColor4f(1, 1, 1, 0.5f);
					glTexCoord2f(-r,     -r    ); glVertex3f(-off + 256.0f * -r,       256.0f * -r,       1);
					glTexCoord2f(r+1.0f, -r    ); glVertex3f(-off + 256.0f * (r+1.0f), 256.0f * -r,       1);
					glTexCoord2f(r+1.0f, r+1.0f); glVertex3f(-off + 256.0f * (r+1.0f), 256.0f * (r+1.0f), 1);
					glTexCoord2f(-r,     r+1.0f); glVertex3f(-off + 256.0f * -r,       256.0f * (r+1.0f), 1);
				}
				glEnd();
			}
			SDL_GL_UnbindTexture(texture);

			if (blend_was_enabled) {
				glEnable(GL_BLEND);
			} else {
				glDisable(GL_BLEND);
			}

			glDisable(GL_FOG);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, PLAY_AREA_W, PLAY_AREA_H, 0);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}
		SDL_RenderFlush(renderer);
	}

}
