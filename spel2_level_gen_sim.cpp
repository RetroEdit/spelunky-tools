/*
Author(s): RetroEdit

Simulate Spelunky 2 level generation
*/

#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

// Uses SemVer 2.0.0
std::uint16_t VERSION_MAJOR = 0;
std::uint16_t VERSION_MINOR = 3;
std::uint16_t VERSION_PATCH = 1;
std::string VERSION_STRING = std::to_string(VERSION_MAJOR) + "." +
	std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);

inline uint64_t rotl64 (uint64_t n, unsigned int c) {
	const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
	c &= mask;
	return (n << c) | (n >> ((-c) & mask));
}

const std::uint64_t PRNG_M0 = 0x9E6C63D0676A9A99;
const std::uint64_t PRNG_M1 = 0xD3833E804F4C574B;

inline std::uint64_t shift_prng_value(std::uint64_t a) {
	return ((a >> 0x33) ^ a ^ (a >> 0x17));
}

inline void next_prng(std::uint64_t& a, std::uint64_t& b) {
	auto a0 = a;
	a = b * PRNG_M1;
	b = rotl64(b - a0, 0x1B);
}

inline std::uint32_t randint(const std::uint64_t prng_a, std::uint32_t a, std::uint32_t b) {
	return (((prng_a & 0xFFFFFFFF) * (b - a + 1)) >> 0x20) + a;
}

// TODO: Using a #define for this is probably unnecessary
#define GET_CO_SUBTHEMES true

#if GET_CO_SUBTHEMES
// const char* CO_SUBTHEMES[] = {"Dwelling", "Jungle", "Volcana", "Tide Pool",
	// "Temple", "Ice Caves", "Neo Babylon", "Sunken City"};
#endif

const int W1_LEVELS = 4;
const int W2_LEVELS = 4;
const int W3_LEVELS = 1;
const int W4_LEVELS = 4;
const int W5_LEVELS = 1;
const int W6_LEVELS = 4;
const int NON_W7_LEVELS = (
	W1_LEVELS + W2_LEVELS + W3_LEVELS + W4_LEVELS + W5_LEVELS + W6_LEVELS
);
const int W7_LEVELS = 98;

#if GET_CO_SUBTHEMES
std::uint16_t analyze_seed_co(const std::uint32_t seed, int* co_subtheme_counts) {
#else
std::uint16_t analyze_seed_co(const std::uint32_t seed) {
#endif
	// FIXME: Premature scoping optimization
	std::uint16_t total_co_rooms = 0;

	std::uint64_t a = 0;
	std::uint64_t b = 0;
	std::uint64_t c = 0;

	std::uint64_t PRNG_SESSION_SEED = 0;
	std::uint64_t PRNG_LEVEL_SEED = 0;

	// int world = 0;
	bool dark_level_in_world = false;
	bool dark_level_rolled = false;

	#ifdef GET_CO_SUBTHEMES
	int co_subtheme = 0;
	#endif
	int width = 0;
	int height = 0;

	total_co_rooms = 0;

	// init_prng
	c = 0;
	if (seed == 0) {
		c = 1;
	}
	b = (c - seed) * PRNG_M0;
	b = shift_prng_value(b) * PRNG_M0;
	a = (rotl64(b, 0x1B) * PRNG_M0) | 1;
	b = shift_prng_value(b) * PRNG_M1;
	// TODO: should b be converted to int32 here?
	c = 0;
	if (b == 0) {
		c = 1;
	}
	b = (c - b) * PRNG_M0;
	b = shift_prng_value(b) * PRNG_M0;
	b = rotl64(b, 0x1B) * PRNG_M0;

	PRNG_SESSION_SEED = a;
	PRNG_LEVEL_SEED = a + b;

	for (int level = 2; level <= NON_W7_LEVELS; ++level) {
		a = PRNG_SESSION_SEED;
		b = PRNG_LEVEL_SEED;
		b = a + b;
		PRNG_LEVEL_SEED = b;
	}

	// world = 7;
	dark_level_in_world = false;
	for (int level = 1; level <= W7_LEVELS; ++level) {
		a = PRNG_SESSION_SEED;
		b = PRNG_LEVEL_SEED;
		b = a + b;
		PRNG_LEVEL_SEED = b;

		b = (b >> 0x20) ^ b;

		b = b & 0xFFFFFFFF;
		c = 0;
		if (b == 0) {
			c = 1;
		}
		b = (c - b) * PRNG_M0;
		b = shift_prng_value(b) * PRNG_M0;
		a = rotl64(b, 0x1B) * PRNG_M0;
		b = shift_prng_value(b);

		// Technically the game runs next_prng(a, b) 10 times
		// But the game doesn't use the last result, so here it's 9
		// Replace with offset >= 0 for 10 iterations
		for (int offset = 9; offset > 0; --offset) {
			next_prng(a, b);
		}

		// FIXME: Copied from above
		b = a & 0xFFFFFFFF;
		c = 0;
		if (b == 0) {
			c = 1;
		}
		b = (c - b) * PRNG_M0;
		b = shift_prng_value(b) * PRNG_M0;
		a = rotl64(b, 0x1B) * PRNG_M0;
		b = shift_prng_value(b);

		if (level >= 5) {
			#if GET_CO_SUBTHEMES
			co_subtheme = randint(a, 0, 7);
			co_subtheme_counts[co_subtheme] += 1;
			#endif
			next_prng(a, b);
		}

		if (not dark_level_in_world) {
			dark_level_rolled = randint(a, 0, 11) == 0;
			next_prng(a, b);
			if (dark_level_rolled) {
				dark_level_in_world = true;
			}
		}

		if (level >= 5) {
			for (int i = 0; i < 2; ++i) {
				next_prng(a, b);
			}
			width = randint(a, 5, 8);
			next_prng(a, b);
			height = randint(a, 4, 8);
			// next_prng(a, b);
			// print(f"{world}-{level:0>2}: {CO_SUBTHEMES[co_subtheme]:>11} {width}x{height}")
			// print(f"{world}-{level}: {CO_SUBTHEMES[co_subtheme]} {width}x{height}")
			// print(f"{world}-{level:0>2}: {CO_SUBTHEMES[co_subtheme]:>11} {width}x{height} {int(dark_level_rolled)}")

			// -2 because CO levels have empty "rooms" on all sides
			total_co_rooms += (width-2)*(height-2);
		}
	}
	return total_co_rooms;
}

void search_lowco(const std::uint32_t start_seed, std::uint64_t max_seed) {
	auto start_time = std::chrono::steady_clock::now();

	// Mean seed size is 1692 (maybe)
	const std::uint16_t SMALL_SIZE = 1400;
	const std::uint16_t BIG_SIZE = 2000;
	const std::uint32_t MAX_SEEDS = 250000;
	std::uint32_t* small_co_seeds = new std::uint32_t[MAX_SEEDS] ();
	std::uint16_t* small_co_sizes = new std::uint16_t[MAX_SEEDS] ();
	std::uint32_t* big_co_seeds = new std::uint32_t[MAX_SEEDS] ();
	std::uint16_t* big_co_sizes = new std::uint16_t[MAX_SEEDS] ();
	std::uint32_t num_small_seeds = 0;
	std::uint32_t num_big_seeds = 0;

	if (max_seed > 0xFFFFFFFF) {
		max_seed = 0xFFFFFFFF;
	}

	std::uint16_t total_co_rooms = 0;
	std::uint64_t seed = start_seed;
	for (; seed <= max_seed; ++seed) {
		#if GET_CO_SUBTHEMES
		int co_subtheme_counts[8] {0};
		total_co_rooms = analyze_seed_co(seed, co_subtheme_counts);
		#else
		total_co_rooms = analyze_seed_co(seed);
		#endif

		if (total_co_rooms <= SMALL_SIZE) {
			small_co_seeds[num_small_seeds] = seed;
			small_co_sizes[num_small_seeds] = total_co_rooms;
			++num_small_seeds;
			if (num_small_seeds >= MAX_SEEDS) {
				break;
			}
		}
		if (total_co_rooms >= BIG_SIZE) {
			big_co_seeds[num_big_seeds] = seed;
			big_co_sizes[num_big_seeds] = total_co_rooms;
			++num_big_seeds;
			if (num_big_seeds >= MAX_SEEDS) {
				break;
			}
		}
	}

	// std::string filename = "co_seeds.csv";
	// std::ofstream file(filename);
	FILE *file = std::fopen("co_seeds.csv", "w");

	// printf("Small seeds:\n");
	for (uint32_t i = 0; i < num_small_seeds; ++i) {
		fprintf(file, "%08X,%i\n", small_co_seeds[i], small_co_sizes[i]);
	}
	// printf("Big seeds:\n");
	for (uint32_t i = 0; i < num_big_seeds; ++i) {
		fprintf(file, "%08X,%i\n", big_co_seeds[i], big_co_sizes[i]);
	}
	// file:close();
	std::fclose(file);

	delete[] small_co_seeds;
	delete[] small_co_sizes;
	delete[] big_co_seeds;
	delete[] big_co_sizes;

	auto end_time = std::chrono::steady_clock::now();
	// TODO: There's probably a better way
	// strftime seemed promising
	auto elapsed_time = static_cast<long long int>(std::chrono::duration<double>(end_time - start_time).count());
	auto elapsed_d = elapsed_time / (60 * 60 * 24);
	elapsed_time -= (elapsed_d * 60 * 60 * 24);
	auto elapsed_h = elapsed_time / (60 * 60);
	elapsed_time -= (elapsed_h * 60 * 60);
	auto elapsed_m = elapsed_time / 60;
	elapsed_time -= (elapsed_m * 60);
	auto elapsed_s = elapsed_time;
	printf(
		"Seeds %08X-%08X searched in %lli days, %02lli:%02lli:%02lli\n",
		start_seed,
		static_cast<std::uint32_t>(seed - 1),
		elapsed_d,
		elapsed_h,
		elapsed_m,
		elapsed_s
	);
	printf("%u with fewer than %u CO rooms.\n", num_small_seeds, SMALL_SIZE);
	printf("%u with greater than %u CO rooms.\n", num_big_seeds, BIG_SIZE);
}

// Probably a one-use function, so non-generic and less validated
// For multiple uses, I would have genericized parameters
void calculateThemeCounts() {
	printf("Calculating theme counts for seeds read from a file\n");
	printf("and appending them to a new file\n");
	// Some kind of file reading/writing loop
	// TODO: Could check if the file exists
	std::ifstream infile("data/co_seeds_0.2.0.csv");
	std::string line;
	FILE* outfile = std::fopen("co_seeds_0.3.0.csv", "w");
	while (std::getline(infile, line)) {
		std::size_t pos{};
		std::uint32_t seed = std::stoul(line, &pos, 16);
		#if GET_CO_SUBTHEMES
		int co_subtheme_counts[8] {0};
		int total_co_rooms = analyze_seed_co(seed, co_subtheme_counts);
		#else
		int total_co_rooms = analyze_seed_co(seed);
		#endif
		fprintf(outfile, "%08X,%i", seed, total_co_rooms);
		#if GET_CO_SUBTHEMES
		for(int i = 0; i < 8; ++i) {
			fprintf(outfile, ",%2i", co_subtheme_counts[i]);
		}
		#endif
		fprintf(outfile, "\n");
	}
	std::fclose(outfile);
	// infile:close()
}

const char* HEX_DIGITS = "0123456789abcdefABCDEF";

bool is_seed_str(std::string s) {
	bool is_hex = (s.find_first_not_of(HEX_DIGITS) == std::string::npos);
	return is_hex and (s.length() == 8);
}

int main(int argc, char *argv[]) {
	printf("Spelunky 2 level gen simulator %s\n", VERSION_STRING.c_str());
	// if (true) {
		// calculateThemeCounts();
		// return 0;
	// }
	bool validArgs = true;
	// TODO: Really, this should process all args and check if they're each valid
	// Unrecognized arg "<input>"
	// The frontend should probably be a separate file, as well.
	if (argc >= 3) {
		// TODO: int is probably a suboptimal type here; size_t maybe?
		for (int i = 1; i < 3; ++i) {
			if (not is_seed_str(argv[i])) {
				validArgs = false;
				break;
			}
		}
		if (validArgs) {
			std::size_t pos{};
			std::uint32_t start_seed = std::stoul(argv[1], &pos, 16);
			std::uint32_t max_seed = std::stoul(argv[2], &pos, 16);
			search_lowco(start_seed, max_seed);
		}
	}
	else if (argc >= 2) {
		// FIXME: Duplicated code from above
		for (int i = 1; i < 2; ++i) {
			if (not is_seed_str(argv[i])) {
				validArgs = false;
				break;
			}
		}
		if (validArgs) {
			std::size_t pos{};
			std::uint32_t seed = std::stoul(argv[1], &pos, 16);
			#if GET_CO_SUBTHEMES
			int co_subtheme_counts[8] {0};
			std::uint16_t total_co_rooms = analyze_seed_co(seed, co_subtheme_counts);
			#else
			std::uint16_t total_co_rooms = analyze_seed_co(seed);
			#endif
			printf("%08X has %u CO rooms\n", seed, total_co_rooms);
		}
	}
	else {
		validArgs = false;
	}
	if (not validArgs) {
		printf("Usage: level_gen_sim <start_seed> <max_seed>\n");
		printf("Seeds should be 8-character seeds from Spelunky 2\n");
	}
}
