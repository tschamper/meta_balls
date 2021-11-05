#pragma once

#include "SDL.h"
#include<iostream>
#include<vector>

template<int mWidth, int mHeight>
class Screen
{
private:
  SDL_Window* mWindow;
  SDL_Renderer* mRenderer;
  SDL_Texture* mTexture;
public:
  Screen(std::string name);
  ~Screen();
  
  int poll_events();
  void update_texture(const std::vector<std::uint32_t>& pixels);
};

template <int mWidth, int mHeight>
Screen<mWidth, mHeight>::Screen(std::string name)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Could not init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  mWindow = SDL_CreateWindow(name.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mWidth, mHeight, 0);
  if (!mWindow) {
    fprintf(stderr, "Could not create window\n");
    exit(1);
  }
  mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_SOFTWARE);
  if (!mRenderer) {
    fprintf(stderr, "Could not create renderer\n");
    exit(1);
  }
  mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, mWidth, mHeight);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
}

template <int mWidth, int mHeight>
Screen<mWidth, mHeight>::~Screen()
{
  SDL_DestroyWindow(mWindow);
  SDL_Quit();
}
template <int mWidth, int mHeight>
int Screen<mWidth, mHeight>::poll_events()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        return 1;
      }
    case SDL_WINDOWEVENT:
      switch (event.window.event) {
      case SDL_WINDOWEVENT_CLOSE:
        return 1;
      }
    }
  }
}
template <int mWidth, int mHeight>
void Screen<mWidth, mHeight>::update_texture(const std::vector<std::uint32_t>& pixels)
{
  SDL_UpdateTexture(mTexture, nullptr, pixels.data(), mWidth * sizeof(std::uint32_t));
  SDL_RenderClear(mRenderer);
  SDL_RenderCopy(mRenderer, mTexture, nullptr, nullptr);
  SDL_RenderPresent(mRenderer);
}
