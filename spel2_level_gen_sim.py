"""
Author(s): RetroEdit

Simulate Spelunky 2 level generation
"""

from collections import Counter

def uint32(i):
    return i & 0xFFFFFFFF

def uint64(i):
    return i & 0xFFFFFFFFFFFFFFFF

def rol64(i, r):
    return uint64((i << r) | (i >> (64 - r)))

def hex32(i):
    return f'{i:0>8X}'

def hex64(i):
    return f'{i:0>16X}'

# PRNG multipliers
PRNG_M0 = 0x9E6C63D0676A9A99
PRNG_M1 = 0xD3833E804F4C574B

# TODO: Make it an Enum
CO_SUBTHEMES = ['Dwelling', 'Jungle', 'Volcana', 'Tide Pool',
    'Temple', 'Ice Caves', 'Neo Babylon', 'Sunken City']

def init_prng(seed):
    c = 0 if seed != 0 else 1
    b = uint64((c - seed) * PRNG_M0)
    a = uint64(((b >> 0x33) ^ b ^ (b >> 0x17)) * PRNG_M0)
    b = uint64(((a >> 0x33) ^ a ^ (a >> 0x17)) * PRNG_M1)
    a = uint64(rol64(a, 0x1B) * PRNG_M0) | 1
    c = 0 if b != 0 else 1
    b = uint64((c - b) * PRNG_M0)
    b = uint64(((b >> 0x33) ^ b ^ (b >> 0x17)) * PRNG_M0)
    b = uint64(rol64(b, 0x1B) * PRNG_M0)
    # rdx,rsi
    return a, b

def next_prng(a, b):
    """Advances the prng state"""
    return uint64(b * PRNG_M1), rol64(uint64(b - a), 0x1B)

def randint(prng_a, stop_value):
    """Returns a uint in [0, stop_value - 1]"""
    return (uint32(prng_a) * stop_value) >> 0x20

def get_num_co_rooms(seed, print_level_info=False):
    total_co_rooms = 0

    # First level initializes the prng variables
    a, b = init_prng(seed)
    PRNG_SESSION_SEED = a
    PRNG_LEVEL_SEED = uint64(a + b)
    # 1+21 levels before CO
    for l in range(21 - 4):
        a, b = PRNG_SESSION_SEED, PRNG_LEVEL_SEED
        b = uint64(a + b)
        PRNG_LEVEL_SEED = b

    world = 7
    dark_level_in_world = False
    for level in range(5 - 4, 98+1):
        a, b = PRNG_SESSION_SEED, PRNG_LEVEL_SEED
        b = uint64(a + b)
        PRNG_LEVEL_SEED = b
        # print(f'{l:>2}', hex64(PRNG_LEVEL_SEED))

        b = (b >> 0x20) ^ b

        b = uint32(b)
        c = 0 if b != 0 else 1
        b = uint64((c - b) * PRNG_M0)
        b = uint64(((b >> 0x33) ^ b ^ (b >> 0x17)) * PRNG_M0)
        a = uint64(rol64(b, 0x1B) * PRNG_M0)
        b = (b >> 0x33) ^ b ^ (b >> 0x17)

        # Initialize all of the prng buffers
        # (skips part of the steps)
        for offset in range(9, -1, -1):
            a0 = a
            a, b = next_prng(a, b)

        # FIXME: Copied from above
        b = uint32(a0)
        c = 0 if b != 0 else 1
        b = uint64((c - b) * PRNG_M0)
        b = uint64(((b >> 0x33) ^ b ^ (b >> 0x17)) * PRNG_M0)
        a = uint64(rol64(b, 0x1B) * PRNG_M0)
        b = (b >> 0x33) ^ b ^ (b >> 0x17)

        # Note: This condition might not strictly match the game's logic
        if level >= 5:
            co_subtheme = uint32(a) >> 0x1D
            a, b = next_prng(a, b)

        if not dark_level_in_world:
            dark_level_rolled = randint(a, 12) == 0
            a, b = next_prng(a, b)
            if dark_level_rolled:
                dark_level_in_world = True

        if level >= 5:
            for _ in range(2):
                a, b = next_prng(a, b)

            width = randint(a, 4) + 5
            a, b = next_prng(a, b)
            height = randint(a, 5) + 4
            a, b = next_prng(a, b)
            if print_level_info:
                print(f"{world}-{level:0>2}: {CO_SUBTHEMES[co_subtheme]:>11} {width}x{height}")
            # print(f"{world}-{level}: {CO_SUBTHEMES[co_subtheme]} {width}x{height}")
            # print(f"{world}-{level:0>2}: {CO_SUBTHEMES[co_subtheme]:>11} {width}x{height} {int(dark_level_rolled)}")

            # -2 because CO levels have empty "rooms" on all sides
            total_co_rooms += (width-2)*(height-2)
    # print(hex32(seed), total_co_rooms)

    # print(f"{seed:0>8X}: {total_co_rooms}")
    return total_co_rooms

def search_lowco(start_seed, num_seeds):
    fewest_co_rooms, fewest_co_rooms_seed = (6 * 6) * 94, None
    sizes = [None] * num_seeds

    PRNG_SESSION_SEED, PRNG_LEVEL_SEED = None, None
    for seed in range(start_seed, start_seed + num_seeds):
        total_co_rooms = get_num_co_rooms(seed)
        if total_co_rooms <= fewest_co_rooms:
            fewest_co_rooms = total_co_rooms
            fewest_co_rooms_seed = seed
        sizes[seed-start_seed] = total_co_rooms
    print(f"Best result after searching seeds {hex32(start_seed)}-{hex32(start_seed+num_seeds-1)} ({num_seeds} seeds):")
    print(f"{hex32(fewest_co_rooms_seed)}: {fewest_co_rooms}")

    # TEMPORARY
    return sizes

def process_co_seeds():
    seed_sizes = Counter()

    with open('co_seeds.csv', 'rt') as infile:
        for line in infile:
            if not line:
                continue
            seed, num_co_rooms = line.split(',')
            seed = int(seed, base=16)
            num_co_rooms = int(num_co_rooms)
            seed_sizes[num_co_rooms] += 1
    print(sorted(seed_sizes.items()))

if __name__ == '__main__':
    # start_seed, num_seeds = 0, 0x10000
    # search_lowco(start_seed, num_seeds)

    # process_co_seeds()

    seed = 0x00005365
    total_co_rooms = get_num_co_rooms(seed, print_level_info=True)
    print(f"{hex32(seed)}: {total_co_rooms}")
