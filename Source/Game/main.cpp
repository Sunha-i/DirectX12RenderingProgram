#include "Common.h"

#include "Game/Game.h"
#include "Light/RotatingPointLight.h"
#include "Cube/Cube.h"
#include "Cube/RotatingCube.h"
#include "Model/Model.h"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
    std::unique_ptr<Game> game = std::make_unique<Game>(L"Dx12 Renderer");

    // Point Light
    XMFLOAT4 color(0.9f, 0.9f, 0.9f, 1.0f);
    std::shared_ptr<PointLight> pointLight = std::make_shared<PointLight>(XMFLOAT4(-0.577f, 0.577f, -0.577f, 0.0f), color);
    if (FAILED(game->GetRenderer()->AddPointLight(0u, pointLight)))
        return 0;

    // Point Light Cube
    std::shared_ptr<Cube> lightCube = std::make_shared<Cube>(color);
    lightCube->Scale(0.3f, 0.3f, 0.3f);
    lightCube->Translate(5.0f * XMVectorSet(-0.577f, 0.577f, -0.577f, 0.0f));
    if (FAILED(game->GetRenderer()->AddRenderable(L"LightCube", lightCube)))
        return 0;

    // Rotating Point Light
    XMFLOAT4 Rcolor(0.8f, 0.0f, 0.0f, 1.0f);
    std::shared_ptr<RotatingPointLight> RpointLight = std::make_shared<RotatingPointLight>(XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f), Rcolor);
    if (FAILED(game->GetRenderer()->AddPointLight(1u, RpointLight)))
        return 0;

    // Rotating Point Light Cube
    std::shared_ptr<RotatingCube> RlightCube = std::make_shared<RotatingCube>(Rcolor);
    RlightCube->Translate(5.0f * XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f));
    if (FAILED(game->GetRenderer()->AddRenderable(L"RotatingLightCube", RlightCube)))
        return 0;

    // Stone Model
    std::shared_ptr<Model> stoneGolem = std::make_shared<Model>(L"../../Data/Stone/Stone.obj");
    stoneGolem->Scale(0.5f, 0.5f, 0.5f);
    if (FAILED(game->GetRenderer()->AddRenderable(L"StoneGolem", stoneGolem)))
        return 0;

    if (FAILED(game->Initialize(hInstance, nCmdShow)))
    {
        return 0;
    }

    return game->Run();
}