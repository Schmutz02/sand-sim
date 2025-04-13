#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include <windowsx.h>

#define DEG2RAD(degrees) (degrees / 180.0f)

#define WIDTH 1280
#define HEIGHT 720
#define MAX_PARTICLE_COUNT (WIDTH * HEIGHT)

#define ASSERT(expression) {if (!(expression)) {*(int*)0 = 0;}}

typedef struct
{
    int x, y;
    uint32_t color;
    bool active;
} Particle;

uint32_t backbuffer[WIDTH * HEIGHT];
Particle* collision_map[WIDTH * HEIGHT];
Particle particles[MAX_PARTICLE_COUNT];

// https://www.rapidtables.com/convert/color/hsv-to-rgb.html
// NOTE: hue [0, 360) saturation and brightness [0, 1]
static uint32_t
hsb_to_rgb(float hue, float saturation, float brightness)
{
    ASSERT(hue >= 0.0f);
    ASSERT(hue < 360.0f);
    ASSERT(saturation >= 0.0f);
    ASSERT(saturation <= 1.0f);
    ASSERT(brightness >= 0.0f);
    ASSERT(brightness <= 1.0f);

    float c = brightness * saturation;
    float x = c * (1.0f - abs(((int)(hue / 60.0f) & 1) - 1.0f));
    float m = brightness - c;

    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (hue < 60.0f)
    {
        r = c;
        g = x;
    }
    else if (hue < 120.0f)
    {
        r = x;
        g = c;
    }
    else if (hue < 180.0f)
    {
        g = c;
        b = x;
    }
    else if (hue < 240.0f)
    {
        g = x;
        b = c;
    }
    else if (hue < 300.0f)
    {
        b = c;
        r = x;
    }
    else if (hue < 360.0f)
    {
        b = x;
        r = c;
    }

    return (uint32_t)((r + m) * 255.0f) << 16 |
           (uint32_t)((g + m) * 255.0f) << 8 |
           (uint32_t)((b + m) * 255.0f) << 0;
}

static bool
is_valid(int x, int y)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return false;
    }

    return collision_map[y * WIDTH + x] == 0;
}

// NOTE: returns -1, 0, or 1
static int
rand_offset(void)
{
    int r = rand();
    if (((float)r / (float)RAND_MAX) < 0.9f)
    {
        return 0;
    }

    return r & 1 ? 1 : -1;
}

static void
put_pixel(int x, int y, uint32_t color)
{
    ASSERT(x >= 0);
    ASSERT(x < WIDTH);
    ASSERT(y >= 0);
    ASSERT(y < HEIGHT);

    backbuffer[y * WIDTH + x] = color;
}

static void
update_particle(Particle* particle)
{
    collision_map[particle->y * WIDTH + particle->x] = 0;

    int wind_offset = rand_offset();
    if (is_valid(particle->x + wind_offset, particle->y))
    {
        particle->x += wind_offset;
    }

    if (is_valid(particle->x, particle->y + 1))
    {
        particle->y += 1;
    }
    else
    {
        if (rand() & 1)
        {
            if (is_valid(particle->x - 1, particle->y) &&
                is_valid(particle->x - 1, particle->y + 1))
            {
                particle->x += -1;
                particle->y += 1;
            }
            else if (is_valid(particle->x + 1, particle->y) &&
                     is_valid(particle->x + 1, particle->y + 1))
            {
                particle->x += 1;
                particle->y += 1;
            }
            else
            {
                particle->active = false;
                collision_map[particle->y * WIDTH + particle->x] = particle;
            }
        }
        else
        {
            if (is_valid(particle->x + 1, particle->y) &&
                is_valid(particle->x + 1, particle->y + 1))
            {
                particle->x += 1;
                particle->y += 1;
            }
            else if (is_valid(particle->x - 1, particle->y) &&
                     is_valid(particle->x - 1, particle->y + 1))
            {
                particle->x += -1;
                particle->y += 1;
            }
            else
            {
                particle->active = false;
                collision_map[particle->y * WIDTH + particle->x] = particle;
            }
        }
    }
}

extern int APIENTRY
WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, int show_command)
{
#if defined(DEBUG)
    srand(0);
#else
    srand(time(0));
#endif
    rand();

    WNDCLASSA window_class = { 0 };
    window_class.lpfnWndProc = DefWindowProcA;
    window_class.hInstance = instance;
    window_class.hCursor = LoadImageA(0,
                                      MAKEINTRESOURCEA(IDC_ARROW),
                                      IMAGE_CURSOR,
                                      0,
                                      0,
                                      LR_DEFAULTSIZE | LR_SHARED);
    window_class.lpszClassName = "some_window_class_name";

    if (!RegisterClassA(&window_class))
    {
        return GetLastError();
    }

    HWND window_handle = CreateWindowA(window_class.lpszClassName,
                                       "Sand",
                                       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       0,
                                       0,
                                       instance,
                                       0);

    if (!window_handle)
    {
        return GetLastError();
    }

    BITMAPINFO backbuffer_info = { 0 };
    backbuffer_info.bmiHeader.biSize = sizeof(backbuffer_info.bmiHeader);
    backbuffer_info.bmiHeader.biWidth = WIDTH;
    backbuffer_info.bmiHeader.biHeight = -HEIGHT;
    backbuffer_info.bmiHeader.biPlanes = 1;
    backbuffer_info.bmiHeader.biBitCount = 32;
    backbuffer_info.bmiHeader.biCompression = BI_RGB;

    int particle_count = 0;
    int mouseX = 0, mouseY = 0;

    while (IsWindow(window_handle))
    {
        RECT clientRect;
        GetClientRect(window_handle, &clientRect);

        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_MOUSEMOVE)
            {
                mouseX = ((float)GET_X_LPARAM(message.lParam) / (float)clientRect.right) * WIDTH;
                mouseY = ((float)GET_Y_LPARAM(message.lParam) / (float)clientRect.bottom) * HEIGHT;
            }
            else
            {
                DispatchMessageA(&message);
            }
        }

        // clear background
        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                put_pixel(x, y, 0x022b5a);
            }
        }

        // update particles and draw
        for (int i = 0; i < MAX_PARTICLE_COUNT; i++)
        {
            if (particles[i].active)
            {
                update_particle(&particles[i]);
            }

            put_pixel(particles[i].x, particles[i].y, particles[i].color);
        }

        // add particles
        if ((GetKeyState(VK_LBUTTON) & 0x8000) && is_valid(mouseX, mouseY) &&
            particle_count < MAX_PARTICLE_COUNT)
        {
            Particle* particle = &particles[particle_count++];
            particle->x = mouseX;
            particle->y = mouseY;
            particle->color = 0xef00;
            particle->active = true;
        }

        HDC window_DC = GetDC(window_handle);
        StretchDIBits(window_DC,
                      0,
                      0,
                      clientRect.right,
                      clientRect.bottom,
                      0,
                      0,
                      WIDTH,
                      HEIGHT,
                      backbuffer,
                      &backbuffer_info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        ReleaseDC(window_handle, window_DC);
    }

    return 0;
}
