#include "lsystem.c"

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

#define ASSERT(expression) {if (!expression) {*(int*)0 = 0;}}

typedef struct
{
    float x, y;
    uint32_t color;
    bool done;
} Particle;

uint32_t clear_data[WIDTH * HEIGHT];
uint32_t backbuffer[WIDTH * HEIGHT];
bool collision_map[WIDTH * HEIGHT];
Particle particles[MAX_PARTICLE_COUNT];

// https://www.rapidtables.com/convert/color/hsv-to-rgb.html
// NOTE: hue [0, 360) saturation and brightness [0, 1]
static uint32_t
hsb_to_rgb(float hue, float saturation, float brightness)
{
    float c = brightness * saturation;
    float x = c * (1 - abs(((int)(hue / 60) & 1) - 1));
    float m = brightness - c;

    float r = 0, g = 0, b = 0;
    if (hue < 60)
    {
        r = c;
        g = x;
    }
    else if (hue < 120)
    {
        r = x;
        g = c;
    }
    else if (hue < 180)
    {
        g = c;
        b = x;
    }
    else if (hue < 240)
    {
        g = x;
        b = c;
    }
    else if (hue < 300)
    {
        b = c;
        r = x;
    }
    else if (hue < 360)
    {
        b = x;
        r = c;
    }

    return (int)((r + m) * 255) << 16 |
        (int)((g + m) * 255) << 8 |
        (int)((b + m) * 255) << 0;
}

static bool
is_valid(float x, float y)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return false;
    }

    return !collision_map[(int)y * WIDTH + (int)x];
}

// NOTE: returns -1, 0, or 1
static int
flow_at(float x, float y)
{
    if ((rand() / (float)RAND_MAX) < 0.9f)
    {
        return 0;
    }

    return rand() & 1 ? 1 : -1;
}

// NOTE: no bounds checking
static void
put_pixel(float x, float y, uint32_t color)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return;
    }

    backbuffer[(int)y * WIDTH + (int)x] = color;
}

static int32_t
round_float_to_int32(float x)
{
    return (int32_t)(x + 0.5f);
}

static void
populate_pre_data(float x, float y, uint32_t color)
{
    if (x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT)
    {
        return;
    }

    clear_data[(int)y * WIDTH + (int)x] = color;
    collision_map[(int)y * WIDTH + (int)x] = true;
}

static void
pll(float x0, float y0, float x1, float y1, uint32_t color)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float yi = 1;
    if (dy < 0)
    {
        yi = -1;
        dy = -dy;
    }

    float D = (2 * dy) - dx;
    float y = y0;

    int32_t x0int = round_float_to_int32(x0);
    int32_t x1int = round_float_to_int32(x1);
    for (int32_t x = x0int; x <= x1int; x++)
    {
        populate_pre_data(x, y, color);
        if (D > 0)
        {
            y = y + yi;
            D = D + (2 * (dy - dx));
        }
        else
        {
            D = D + 2 * dy;
        }
    }
}

static void
plh(float x0, float y0, float x1, float y1, uint32_t color)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float xi = 1;
    if (dx < 0)
    {
        xi = -1;
        dx = -dx;
    }

    float D = (2 * dx) - dy;
    float x = x0;

    int32_t y0int = round_float_to_int32(y0);
    int32_t y1int = round_float_to_int32(y1);
    for (int32_t y = y0int; y <= y1int; y++)
    {
        populate_pre_data(x, y, color);
        if (D > 0)
        {
            x = x + xi;
            D = D + (2 * (dx - dy));
        }
        else
        {
            D = D + 2 * dx;
        }
    }
}

// TODO: not optimized, taken straight from wikipedia
// NOTE: does physics too
static void
draw_line_to_pre_data(float x0, float y0, float x1, float y1, uint32_t color)
{
    if (abs(y1 - y0) < abs(x1 - x0))
    {
        if (x0 > x1)
            pll(x1, y1, x0, y0, color);
        else
            pll(x0, y0, x1, y1, color);
    }
    else
    {
        if (y0 > y1)
            plh(x1, y1, x0, y0, color);
        else
            plh(x0, y0, x1, y1, color);
    }
}

typedef struct
{
    float x;
    float y;
    float angle;
    float length;
} LSystemState;

extern int APIENTRY
WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, int show_command)
{
#if defined(DEBUG)
    srand(0);
#else
    srand(time(0));
#endif
    rand();

    Transformation transformations[] = {
        {'X', "F+[[X]-X]-F[-FX]+X"},
        {'F', "FF"},
    };

    LSystemInfo lsystem_info = { 0 };
    lsystem_info.seed = "-X";
    lsystem_info.transformations = transformations;
    lsystem_info.transformation_count = 2;

    char* lsystem_memory = VirtualAlloc(0, 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    char* lsystem_memory2 = VirtualAlloc(0, 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    generate_lsystem(&lsystem_info, lsystem_memory);

    lsystem_info.seed = lsystem_memory;
    generate_lsystem(&lsystem_info, lsystem_memory2);

    lsystem_info.seed = lsystem_memory2;
    generate_lsystem(&lsystem_info, lsystem_memory);

    lsystem_info.seed = lsystem_memory;
    generate_lsystem(&lsystem_info, lsystem_memory2);

    lsystem_info.seed = lsystem_memory2;
    generate_lsystem(&lsystem_info, lsystem_memory);

    lsystem_info.seed = lsystem_memory;
    generate_lsystem(&lsystem_info, lsystem_memory2);

    LSystemState state = { 0 };
    state.x = (rand() % WIDTH) / 2 + WIDTH / 4;
    state.y = HEIGHT;
    state.angle = DEG2RAD(270);
    state.length = 4;
    int next_state_stack_index = 0;

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            clear_data[i * WIDTH + j] = 0x022b5a;
        }
    }

    for (int i = 0; i < string_length(lsystem_memory2); i++)
    {
        switch (lsystem_memory2[i])
        {
        case 'F':
        {
            float endX = state.x + cosf(state.angle) * state.length;
            float endY = state.y - sinf(state.angle) * state.length;
            draw_line_to_pre_data(state.x, state.y, endX, endY, 0);
            state.x = endX;
            state.y = endY;
            break;
        }
        case '+':
        {
            state.angle += DEG2RAD(25);
            break;
        }
        case '-':
        {
            state.angle -= DEG2RAD(25);
            break;
        }
        case '[':
        {
            ((LSystemState*)lsystem_memory)[next_state_stack_index++] = state;
            break;
        }
        case ']':
        {
            state = ((LSystemState*)lsystem_memory)[--next_state_stack_index];
            break;
        }
        default:
            break;
        }
    }

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

    if (RegisterClassA(&window_class))
    {
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

        if (window_handle)
        {
            BITMAPINFO backbuffer_info = { 0 };
            backbuffer_info.bmiHeader.biSize = sizeof(backbuffer_info.bmiHeader);
            backbuffer_info.bmiHeader.biWidth = WIDTH;
            backbuffer_info.bmiHeader.biHeight = -HEIGHT;
            backbuffer_info.bmiHeader.biPlanes = 1;
            backbuffer_info.bmiHeader.biBitCount = 32;
            backbuffer_info.bmiHeader.biCompression = BI_RGB;

            for (int i = 0; i < MAX_PARTICLE_COUNT; i++)
            {
                float x = rand() % WIDTH;
                particles[i].x = x / 2 + WIDTH / 4;

                float hue = (((int)(x / WIDTH * 4)) * 90);
                particles[i].color = hsb_to_rgb(hue, (rand() / (float)RAND_MAX) * 0.5f + 0.5f, 1);
            }

            int particle_count = 0;
            int done_particle_count = 0;
            int32_t mouseX = 0, mouseY = 0;

            while (IsWindow(window_handle))
            {
                RECT clientRect;
                GetClientRect(window_handle, &clientRect);


                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_MOUSEMOVE)
                    {
                        mouseX = GET_X_LPARAM(message.lParam) / (float)clientRect.right * WIDTH;
                        mouseY = GET_Y_LPARAM(message.lParam) / (float)clientRect.bottom * HEIGHT;
                    }
                    else
                    {
                        DispatchMessageA(&message);
                    }
                }

                particle_count += 1;
                if (particle_count > MAX_PARTICLE_COUNT)
                {
                    particle_count = MAX_PARTICLE_COUNT;
                }

                // clear background
                for (int i = 0; i < HEIGHT; i++)
                {
                    for (int j = 0; j < WIDTH; j++)
                    {
                        put_pixel(j, i, clear_data[i * WIDTH + j]);
                    }
                }

                // update particles and draw
                for (int i = 0; i < particle_count; i++)
                {
                    Particle* particle = &particles[i];
                    if (particle->done)
                    {
                        put_pixel(particle->x, particle->y, particle->color);
                        continue;
                    }

                    int flow = flow_at(particle->x, particle->y);
                    if (is_valid(particle->x + flow, particle->y) &&
                        is_valid(particle->x + flow, particle->y + 1))
                    {
                        particle->x += flow;
                        particle->y += 1;
                    }
                    else if (is_valid(particle->x, particle->y + 1))
                    {
                        particle->y += 1;
                    }
                    else
                    {
                        // randomly check left or right first
                        if (rand() & 1)
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
                                particle->x -= 1;
                                particle->y += 1;
                            }
                            else
                            {
                                particle->done = true;
                                collision_map[(int)particle->y * WIDTH + (int)particle->x] = true;
                            }
                        }
                        else
                        {
                            if (is_valid(particle->x - 1, particle->y) && 
                                is_valid(particle->x - 1, particle->y + 1))
                            {
                                particle->x -= 1;
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
                                particle->done = true;
                                collision_map[(int)particle->y * WIDTH + (int)particle->x] = true;
                            }
                        }
                    }

                    put_pixel(particle->x, particle->y, particle->color);
                }

                if ((GetKeyState(VK_LBUTTON) & 0x8000) && is_valid(mouseX, mouseY) &&
                    particle_count < MAX_PARTICLE_COUNT)
                {
                    Particle* particle = &particles[particle_count++];
                    particle->x = mouseX;
                    particle->y = mouseY;
                    particle->color = 0xef00;
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
        }
    }
}
