/*
 * Random tools
 */

static int next = 1;

void _set_rand_seed(int seed)
{
        next = seed;
}

int _get_rand(void)
{
        next = ((next * 214013L + 2531011L) >> 16) & 0x7FFF;
        return next;
}
