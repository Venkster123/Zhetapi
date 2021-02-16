#include "global.hpp"

// 'Frame time'
const double delta(1.0/120.0);
const chrono::milliseconds frame((int) (1000 * delta));

// Statistical variables
ofstream fout("results.csv");

// Agent
Environment env(0.95);

// Step through each iteration
void step(strategy *s)
{
	double angle = (*s->h_action)(env.state());
	
	// Create the action force and move
	Vec A = Vec::rarg(env.force, angle);
	
	env.move(A + F(env.position), delta);

	// Gather reward and so on
	double r = env.reward();

	(*s->h_reward)(r, env.state(), env.in_bounds(), s->error);
	
	s->reward += r;
	s->tframes++;
	s->frames++;
	
	if (!env.in_bounds() || s->frames >= 10000) {
		env.reset();

		cout << s->name << "\tFinal reward = " << s->reward
			<< "\tframes last = " << s->frames
			<< "\taverage TD-error = "
			<< s->error/s->frames << endl;

		s->csv << s->tframes << ","
			<< s->reward << ","
			<< s->frames << ","
			<< s->error/s->frames
			<< endl;

		s->reward = 0;
		s->error = 0;
		s->frames = 0;
	}
}

// Main function
int main(int argc, char **argv)
{
	// Open suites file
	ifstream config("config.json");

	// Extract json information
	nlohmann::json suites;

	vector <string> libs;

	config >> suites;

	cout << "Compiling shared files for each strategy..." << endl;

	system("mkdir -p libs");
	for (auto suite : suites["Suites"]) {
		string cc = "g++-8 ";
		string flags = " -fPIC -rdynamic -shared -w -O3 ";
		string libdir = " libs";
		string idirs = " -I ../../engine ";

		string src = suite["Sources"];
		string dst = suite["Lib"];
		
		// Add the strategy name to the collection
		libs.push_back(dst);

		dst = "lib" + dst + ".so";

		string cmd = cc + flags + src + " -o " + libdir + "/" + dst + idirs;

		cout << "\t" << cmd << endl;

		if (system(cmd.c_str())) {
			cout << "Error compiling sources, terminating" << endl;

			exit(-1);
		}
	}

	cout << "Done compiling, moving to loading..." << endl;

	vector <strategy *> strats;

	system("mkdir -p res");
	for (auto name : libs) {
		const char *error;

		string path = "libs/lib" + name + ".so";

		void *handle = dlopen(path.c_str(), RTLD_NOW);

		error = dlerror();

		if (error) {
			cerr << "Cannot load strategy '" << name << "': " << error << '\n';
			
			exit(-1);;
		}

		void *init = dlsym(handle, "init");
		void *action = dlsym(handle, "action");
		void *reward = dlsym(handle, "reward");

		error = dlerror();

		if (error) {
			cerr << "Cannot load strategy '" << name << "': " << error << '\n';
			
			exit(-1);
		}

		strategy *s = new strategy;
		
		s->h_init = (strategy::init_t) init;
		s->h_action = (strategy::action_t) action;
		s->h_reward = (strategy::reward_t) reward;

		if (!s->validate()) {
			cerr << "Error importing strategy '"
				<< name << "': null functions" << endl;

			exit(-1);
		}

		s->open("res/" + name + ".csv");

		strats.push_back(s);

		(*s->h_init)(env);
	
		// Initialize the CSV file
		s->csv << "total_frames,final_reward,frames,avg_error" << endl;

		// Other properties
		s->name = name;
	}

	// Launch the graphing script
	system("python3 statistics.py &");

	while (true) {
		this_thread::sleep_for(frame);

		step(strats[0]);
	}

	return 0;
}
