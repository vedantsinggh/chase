#include "renderer.h"
#include "raygui.h"

using namespace chase;

Renderer::Renderer(){
	this->width  = 800;
	this->height = 800;
	this->title = "Renderer";
	init();
}

Renderer::Renderer(int width, int height, std::string title){
	this->width  = width;
	this->height = height; 
	this->title  = title;
	init();
}

void Renderer::init(){
	SetConfigFlags(FLAG_BORDERLESS_WINDOWED_MODE);
	InitWindow(this->width, this->height, this->title.c_str());
	SetWindowMinSize(250,250);
	m_image = GenImageColor(this->width, this->height, BLACK);

	this->mainCamera = Camera(0.0f, 0.0f, -2.0f);
	this->time = 0.0f;
	this->timer = Timer();
	this->inverseResolutionScale = 1;
	this->m_render = LoadTextureFromImage(this->m_image);
}

void Renderer::onResize() {
    this->timer.resetTimer();
    this->height = GetScreenHeight();
    this->width  = GetScreenWidth();

    ImageResizeNN(&this->m_image, this->width, this->height);

    const int numThreads = 10;
    std::vector<std::thread> threads;
    const float time = GetTime();

    auto renderTask = [&](int start, int end) {
        for (int w = start; w < end; w+=this->inverseResolutionScale){
            for (int h = 0; h < this->height; h+=this->inverseResolutionScale) {
				Color existingColor = GetImageColor(this->m_image, w, h);
                Color color = this->perPixel(w, h, time);
				if (color.r == existingColor.r && color.g == existingColor.g &&
                color.b == existingColor.b) continue;
				ImageDrawPixel(&this->m_image, w, h, color);
            }
        }
    };

    const int workPerThread = this->width / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        int start = i * workPerThread;
        int end = (i == numThreads - 1) ? this->width : start + workPerThread;
        threads.push_back(std::thread(renderTask, start, end));
    }

    for (auto& t : threads) {
        t.join();
    }

	UnloadTexture(this->m_render);
	this->m_render = LoadTextureFromImage(this->m_image);
    this->time = this->timer.getDurationElapsed();
}

void Renderer::render(){

	const int fontSize = 20;
	const int padding  = 10;
	const std::string txt = ("TIME: " + std::to_string(this->time) + "ms");
	const int textSize = MeasureText(txt.c_str(), fontSize);

	BeginDrawing();
	{
		ClearBackground(BLACK);
		DrawTexture(this->m_render, 0, 0, WHITE);
		DrawText(txt.c_str(), width - textSize - padding, padding, fontSize, WHITE);
	}
	EndDrawing();
}

Color Renderer::perPixel(int w, int h, float time) {
    
    const float radius = 0.5;
    const float x = (float)w * 2/width  - 1;
    const float y = (float)h * -2/height + 1;
	const Vec3 spherecenter(0.0f, 0.0f, 0.0f);

    const Vec3 dir(x, y, 1.0f);
	const Vec3 lightdirection(sinf(time), 2.0f, cosf(time));

    const float a = dir.dot(dir); 
    const float b = 2.0f * dir.dot(mainCamera.position - spherecenter);
    const float c = (mainCamera.position - spherecenter).dot(mainCamera.position - spherecenter) - radius * radius;
    const float discriminant = b * b - 4 * a * c;


    if (discriminant >= 0) {
		const float hitnear = (-b - sqrt(discriminant)) / (2 * a);
		const Vec3 hitpoint = mainCamera.position + dir * hitnear;
		const float d = (hitpoint).dot(lightdirection);

		const Color hitcolor = {static_cast<unsigned char>(Utils::clamp(d * 255, 0, 255)),
						  static_cast<unsigned char>(Utils::clamp(d * 255, 0, 255)),
						  static_cast<unsigned char>(Utils::clamp(d * 255, 0, 255)), 255};
        return hitcolor; 
    }

    return LIME;
}

bool Renderer::isRunning(){
	return !WindowShouldClose();
}

bool Renderer::isResized(){
	return IsWindowResized();
}

Renderer::~Renderer(){
	UnloadTexture(this->m_render);
	CloseWindow(); 
}
