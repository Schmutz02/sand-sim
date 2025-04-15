#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include <windowsx.h>

#define DEG2RAD(degrees) (degrees / 180.0f)

#define WIDTH 800
#define HEIGHT 600
#define MAX_PARTICLE_COUNT (WIDTH * HEIGHT)

#define ASSERT(expression) {if (!(expression)) {*(int*)0 = 0;}}

typedef enum
{
    NONE_PARTICLE = 0,
    SAND_PARTICLE = 0xC2BD8C,
    WATER_PARTICLE = 0x5278C5,
    STONE_PARTICLE = 0x3E3E3E,
} ParticleType;

typedef struct
{
    ParticleType type;
    union
    {
        struct
        {
            uint8_t health;
        } water;
    };

    int x, y;
} Particle;

uint32_t backbuffer[WIDTH * HEIGHT];
Particle* collision_map[WIDTH * HEIGHT];
Particle* new_collision_map[WIDTH * HEIGHT];
Particle particles[MAX_PARTICLE_COUNT];

static Particle*
get_particle(int x, int y)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return 0;
    }

    return collision_map[y * WIDTH + x];
}

static bool
is_valid(int x, int y)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return false;
    }

    return get_particle(x, y) == 0;
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

// NOTE: returns true upon successful movement
static bool
apply_gravity(Particle* particle)
{
    ASSERT(particle != 0);

    int wind_offset = rand_offset();
    if (is_valid(particle->x + wind_offset, particle->y + 1))
    {
        particle->x += wind_offset;
        particle->y += 1;

        return true;
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

                return true;
            }
            else if (is_valid(particle->x + 1, particle->y) &&
                     is_valid(particle->x + 1, particle->y + 1))
            {
                particle->x += 1;
                particle->y += 1;

                return true;
            }
            
            return false;
        }
        else
        {
            if (is_valid(particle->x + 1, particle->y) &&
                is_valid(particle->x + 1, particle->y + 1))
            {
                particle->x += 1;
                particle->y += 1;

                return true;
            }
            else if (is_valid(particle->x - 1, particle->y) &&
                     is_valid(particle->x - 1, particle->y + 1))
            {
                particle->x += -1;
                particle->y += 1;

                return true;
            }
            
            return false;
        }
    }

    return false;
}

static void
add_particle(ParticleType type, int x, int y)
{
    
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
    int mouse_x = 0, mouse_y = 0;
    ParticleType particle_brush_type = SAND_PARTICLE;

    while (IsWindow(window_handle))
    {
        RECT clientRect;
        GetClientRect(window_handle, &clientRect);

        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_SYSKEYDOWN || message.message == WM_SYSKEYUP ||
                message.message == WM_KEYDOWN || message.message == WM_KEYUP)
            {
                uint8_t key_code = message.wParam;
                bool key_down = ((message.lParam >> 31) & 1) != 1;

                if (key_code == '1' && key_down)
                {
                    particle_brush_type = SAND_PARTICLE;
                }
                else if (key_code == '2' && key_down)
                {
                    particle_brush_type = WATER_PARTICLE;
                }
                else if (key_code == '3' && key_down)
                {
                    particle_brush_type = STONE_PARTICLE;
                }
            }
            else if (message.message == WM_MOUSEMOVE)
            {
                mouse_x = ((float)GET_X_LPARAM(message.lParam) / (float)clientRect.right) * WIDTH;
                mouse_y = ((float)GET_Y_LPARAM(message.lParam) / (float)clientRect.bottom) * HEIGHT;
            }
            else
            {
                DispatchMessageA(&message);
            }
        }

        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                new_collision_map[y * WIDTH + x] = collision_map[y * WIDTH + x];
            }
        }

        // update particles and draw
        for (int i = 0; i < MAX_PARTICLE_COUNT; i++)
        {
            Particle* particle = &particles[i];

            new_collision_map[particle->y * WIDTH + particle->x] = 0;

            switch (particle->type)
            {
            case SAND_PARTICLE:
            {
                apply_gravity(particle);

                new_collision_map[particle->y * WIDTH + particle->x] = particle;

                break;
            }
            case WATER_PARTICLE:
            {
                apply_gravity(particle);

                new_collision_map[particle->y * WIDTH + particle->x] = particle;

                Particle* below_particle = get_particle(particle->x, particle->y + 1);
                Particle* above_particle = get_particle(particle->x, particle->y - 1);
                if (below_particle != 0 && below_particle->type == SAND_PARTICLE ||
                    above_particle != 0 && above_particle->type == SAND_PARTICLE)
                {
                    particle->water.health--;
                }

                if (particle->water.health <= 0)
                {
                    new_collision_map[particle->y * WIDTH + particle->x] = 0;
                }

                break;
            }
            case STONE_PARTICLE:
            {
                new_collision_map[particle->y * WIDTH + particle->x] = particle;

                break;
            }
            default:
            {
                ASSERT("UNKNOWN PARTICLE TYPE!!!!!!!");

                break;
            }
            }
        }

        // add particles
        if ((GetKeyState(VK_LBUTTON) & 0x8000) &&
            particle_count + 4 <= MAX_PARTICLE_COUNT)
        {
            if (is_valid(mouse_x, mouse_y))
            {
                Particle* particle = &particles[particle_count++];

                particle->x = mouse_x;
                particle->y = mouse_y;
                particle->type = particle_brush_type;
                particle->water.health = 150;

                new_collision_map[particle->y * WIDTH + particle->x] = particle;
            }
            if (is_valid(mouse_x + 1, mouse_y))
            {
                Particle* particle = &particles[particle_count++];

                particle->x = mouse_x + 1;
                particle->y = mouse_y;
                particle->type = particle_brush_type;
                particle->water.health = 150;

                new_collision_map[particle->y * WIDTH + particle->x] = particle;
            }
            if (is_valid(mouse_x + 1, mouse_y + 1))
            {
                Particle* particle = &particles[particle_count++];

                particle->x = mouse_x + 1;
                particle->y = mouse_y + 1;
                particle->type = particle_brush_type;
                particle->water.health = 150;

                new_collision_map[particle->y * WIDTH + particle->x] = particle;
            }
            if (is_valid(mouse_x, mouse_y + 1))
            {
                Particle* particle = &particles[particle_count++];

                particle->x = mouse_x;
                particle->y = mouse_y + 1;
                particle->type = particle_brush_type;
                particle->water.health = 150;

                new_collision_map[particle->y * WIDTH + particle->x] = particle;
            }
        }

        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                Particle* particle = new_collision_map[y * WIDTH + x];

                collision_map[y * WIDTH + x] = particle;

                if (particle == 0)
                {
                    put_pixel(x, y, 0x022b5a);
                }
                else
                {
                    put_pixel(x, y, particle->type);
                }
            }
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
