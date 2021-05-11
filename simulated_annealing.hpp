// 焼きなまし
template<class State> struct SimulatedAnnealing {
	State* state;
	Random* rng;
	double best_score;
	State best_state;

	inline SimulatedAnnealing(State& arg_state, Random& arg_rng) :
		state(&arg_state), rng(&arg_rng), best_score(1e9) {}

	template<double (*temperature_schedule)(const double&)> void optimize(const double time_limit) {
		const double t0 = time();
		double old_score = state->score;
		int iteration = 0;
		while (true) {
			iteration++;
			const double t = time() - t0;
			if (t > time_limit) break;
			const double progress_rate = t / time_limit;

			state->update();
			state->calc_score(progress_rate);
			const double new_score = state->score;
			if (chmin(best_score, new_score)) {
				//cout << "upd! new_score=" << new_score << " progress=" << progress_rate << endl;
				best_state = *state;  // 中にポインタがある場合などは注意する
			}
			const double gain = old_score - new_score;  // 最小化: 良くなったらプラス
			const double temperature = temperature_schedule(t);
			const double acceptance_proba = exp(gain / temperature);
			if (acceptance_proba > rng->random()) {
				// 遷移する
				old_score = new_score;
			}
			else {
				// 遷移しない（戻す）
				state->undo();
			}
		}
		*state = best_state;  // 中にポインタがある場合などは注意する
	}
};


inline double sigmoid(const double& a, const double& x) {
	return 1.0 / (1.0 + exp(-a * x));
}

// f: [0, 1] -> [0, 1]
inline double monotonically_increasing_function(const double& a, const double& b, const double& x) {
	ASSERT(b >= 0.0, "parameter `b` should be positive.");
	// a は -10 ～ 10 くらいまで、 b は 0 ～ 10 くらいまで探せば良さそう

	if (a == 0) return x;
	const double x_left = a > 0 ? -b - 0.5 : b - 0.5;
	const double x_right = x_left + 1.0;
	const double left = sigmoid(a, x_left);
	const double right = sigmoid(a, x_right);
	const double y = sigmoid(a, x + x_left);
	return (y - left) / (right - left);  // left とかが大きい値になると誤差がヤバイ　最悪 0 除算になる  // b が正なら大丈夫っぽい
}

// f: [0, 1] -> [start, end]
inline double monotonic_function(const double& start, const double& end, const double& a, const double& b, const double& x) {
	return monotonically_increasing_function(a, b, x) * (end - start) + start;
}