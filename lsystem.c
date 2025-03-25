typedef struct
{
    char axiom;
    char* new_axiom;
} Transformation;

typedef struct
{
    char* seed;

    Transformation* transformations;
    int transformation_count;
} LSystemInfo;

static int
string_length(char* string)
{
    int accum = 0;
    while (*string++) accum++;
    return accum;
}

static void
string_copy(char* s1, char* s2)
{
    int i = 0;
    while (s2[i])
    {
        s1[i] = s2[i];
        i++;
    }
}

// NOTE: will just crash if not enough memory
static void
generate_lsystem(LSystemInfo* info, char* memory)
{
    int memory_index = 0;
    for (int seed_index = 0; seed_index < string_length(info->seed); seed_index++)
    {
        for (int tfm_index = 0; tfm_index < info->transformation_count; tfm_index++)
        {
            if (info->transformations[tfm_index].axiom == info->seed[seed_index])
            {
                string_copy(&memory[memory_index], info->transformations[tfm_index].new_axiom);
                memory_index += string_length(info->transformations[tfm_index].new_axiom);
                goto next_seed_index;
            }
        }

        memory[memory_index++] = info->seed[seed_index];
    next_seed_index:
        {
        };
    }

    memory[memory_index] = '\0';
}
