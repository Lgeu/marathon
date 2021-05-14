#include "library.hpp"
#include "beam_search.hpp"

using namespace std;
namespace Chokudai005 {
    struct Input {
        int id, N, K;
        Stack<Stack<char, 100>, 100> S;
        Input() : S(100, Stack<char, 100>(100)) {}
        void read() {
            cin >> id >> N >> K;
            rep(y, N) {
                rep(x, N) {
                    cin >> S[y][x];
                    S[y][x] -= '1';
                }
            }
        }
        void print() {
            rep(y, N) {
                rep(x, N) {
                    cout << (int)S[y][x];
                }
                cout << endl;
            }
        }
    };

    struct State {
        const Input& input;
        int turn;
        int score;
        Stack<char, 10000> done;  // 0: 未探索  1: 隣接してるが色が違う  2: 吸収済み
        Stack<Stack<int, 10000>, 9> queues;  // 隣接してるが色が違うやつらの座標
        Stack<char, 10000> answer;  // 現在までの行動

        State(const Input& a_input) :
            input(a_input),
            turn(0),
            score(0),
            done(10000),
            queues(9),
            answer()
        {
            const int& N = input.N;
            const int center = N / 2 * N + N / 2;
            done[center] = 1;
            queues[input.S[N / 2][N / 2]].push(center);
        }

        struct Action {
            char color;
        };
        struct Patch {
            int score;
            char color;
            vector<pair<char, int>> queues;
            vector<pair<int, char>> done;
        };
        struct ReversePatch {
            int score;
            char color;
            vector<int> q;
            array<int, 9> q_count;
            vector<pair<int, char>> done;
        };
        struct NewStateInfo {
            double score;
            Action action;
        };

        template<int max_n_next_states>
        bool GetNextStates(Stack<State::NewStateInfo, max_n_next_states>& next_states) {
            for (char c = 0; c < 9; c++) {
                next_states.push({
                    (double)(-turn + score + queues[c].size()),  // このスコアは少し嘘だが…？
                    Action{c}
                    });
            }
            return score == input.N * input.N;
        }

        pair<Patch, ReversePatch> Do(const Action& action) {
            // 探索済み部分に隣接する action.color の色のマスを再帰的に吸収し、
            // 吸収したマスに隣接する部分を新たにキューに加える
            const auto& color = action.color;
            const auto& N = input.N;
            answer.push(color);  // -
            Patch patch{};
            ReversePatch reverse_patch{};
            reverse_patch.score = score;
            patch.color = reverse_patch.color = color;
            static Stack<int, 10000> stk;
            stk = queues[color];
            reverse_patch.q = stk.ToVector();
            queues[color].clear();  // -
            for (auto ptr_yx = stk.begin(); ptr_yx != stk.end(); ptr_yx++) {
                int yx = *ptr_yx;
                if (done[yx] == 2) continue;
                int y = yx / N, x = yx % N;
                const int c = input.S[y][x];
                if (c == color) {  // 新たに吸収
                    if (turn < 30) {//cout << "Do-kyushu" << endl;
                    }
                    reverse_patch.done.emplace_back(yx, done[yx]);
                    done[yx] = 2;  // -
                    patch.done.emplace_back(yx, done[yx]);
                    score++;  // -
                    if (x != 0) stk.push(yx - 1);
                    if (x != N - 1) stk.push(yx + 1);
                    if (y != 0) stk.push(yx - N);
                    if (y != N - 1) stk.push(yx + N);
                }
                else if (done[yx] == 0) {  // 新たに隣接した違う色のマス
                    reverse_patch.done.emplace_back(yx, done[yx]);
                    done[yx] = 1;  // -
                    patch.done.emplace_back(yx, done[yx]);
                    reverse_patch.q_count[c]++;
                    queues[c].push(yx);  // -
                    patch.queues.emplace_back(c, yx);
                }
            }
            patch.score = score;
            turn++;
            return { patch, reverse_patch };
        }

        void Redo(const Patch& patch) {
            answer.push(patch.color);
            queues[patch.color].clear();
            for (const auto& t : patch.done) {
                done[t.first] = t.second;
            }
            for (const auto& t : patch.queues) {
                queues[t.first].push(t.second);
            }
            score = patch.score;
            turn++;
        }

        void Undo(const ReversePatch& reverse_patch) {
            answer.pop();
            for (const auto& t : reverse_patch.done) {
                ASSERT_RANGE(t.first, 0, done.right);
                done[t.first] = t.second;
            }
            for (char c = 0; c < 9; c++) {
                for (int i = 0; i < reverse_patch.q_count[c]; i++) {
                    queues[c].pop();
                }
            }
            queues[reverse_patch.color] = reverse_patch.q;
            score = reverse_patch.score;
            turn--;
        }
    };

    void solve() {
        static Chokudai005::Input input;
        input.read();
        static const Chokudai005::State state(input);
        static ColunChokudaiSearch<Chokudai005::State, 10000> s(state);
        s.Search(2.4, 10000);
        static auto stt = s.BestState();
        static auto path = s.BestStatePath();
        //cout << stt.score << " " << stt.turn << " " << endl;
        //cout << "n_nodes=" << s.tree.nodes->size() << endl;
        cout << path.size() - 1 << endl;
        for (auto it = path.begin() + 1; it != path.end(); it++) {
            cout << input.N / 2 + 1 << " " << input.N / 2 + 1 << " " << (int)it->color + 1 << endl;
        }
    }
}  // namespace Chokudai005


namespace RCO2018QualA {
    struct Input {
        int N, K, H, W, T;
        array<array<array<char, 50>, 50>, 100> stage;
        array<Vec2<signed char>, 100> initial_players;
        void Read() {
            cin >> N >> K >> H >> W >> T;
            for (int i = 0; i < 100; i++) {
                for (int y = 0; y < 50; y++) {
                    string s;
                    cin >> s;
                    for (int x = 0; x < 50; x++) {
                        stage[i][y][x] = s[x];
                        if (s[x] == '@') initial_players[i] = Vec2<signed char>(y, x);
                    }
                }
            }
        }
    };

    Input input;
    array<unsigned, 66000> hashes;
    constexpr array<Vec2<signed char>, 4> dyxs{ Vec2<signed char>{1, 0}, {0, 1}, {-1, 0}, {0, -1} };
    array<int, 8> choosen_stages{ 8, 9, 10, 11, 12, 13, 14, 15 };

    void choose_stages() {
        Random rng(42);
        array<int, 100> scores{};
        
        for (int i = 0; i < 100; i++) {
            auto stack = Stack<Vec2<signed char>, 2500>();
            stack.push(input.initial_players[i]);
            auto closed = array<bitset<50>, 50>();
            while (!stack.empty()) {
                auto vyx = stack.pop();
                for (int d = 0; d < 4; d++) {
                    const auto& dyx = dyxs[d];
                    auto uyx = vyx + dyx;
                    if (input.stage[i][uyx.y][uyx.x] == 'o' && !closed[uyx.y][uyx.x]) {
                        stack.push(uyx);
                        scores[i]++;
                        closed[uyx.y][uyx.x] = true;
                    }
                }
            }
        }

        auto idxs = array<int, 100>();
        iota(idxs.begin(), idxs.end(), 0);
        sort(idxs.begin(), idxs.end(), [&](const int& l, const int& r) { return scores[l] > scores[r]; });
        for (int i = 0; i < 8; i++) {
            choosen_stages[i] = idxs[i];
        }
    }

    template<int h=50, int w=50>
    struct BitBoard {
        bitset<h * w> data;
        inline BitBoard() = default;
        inline bool Get(const Vec2<signed char>& p) const {
            return data[(int)p.y * 50 + (int)p.x];
        }
        inline void Set(const Vec2<signed char>& p, const bool& val) {
            data[(int)p.y * 50 + (int)p.x] = val;
        }
    };

    struct Command {
        bitset<2500 * 2> data;
        inline Command() = default;
        inline void Set(const int& idx, const int& val) {
            ASSERT_RANGE(idx, 0, 2500);
            ASSERT_RANGE(val, 0, 4);
            data[idx * 2] = val & 1;
            data[idx * 2 + 1] = val >> 1;
        }
        inline void Print() const {
            for (int i = 0; i < 2500; i++) {
                cout << "DRUL"[(int)data[i * 2 + 1] << 1 | (int)data[i * 2]];
            }
            cout << endl;
        }
    };

    struct State {
        double score;
        bool termination;
        signed hash;
        short turn;
        array<BitBoard<>, 8> visited;
        array<Vec2<signed char>, 8> players;
        Command command;
        
        inline State() :
            score(0.0), termination(false), hash(0u), turn(0), visited(), players(), command()
        {
            for (int i = 0; i < 8; i++) {
                players[i] = input.initial_players[choosen_stages[i]];
            }
        }
        struct Action {
            signed char d;
        };
        struct NewStateInfo {
            double score;
            Action action;
            unsigned hash;
        };
        inline void GetNextStates(Stack<NewStateInfo, 10000>& res) {
            for (signed char d = 0; d < 4; d++) {
                const auto& dyx = dyxs[d];
                double new_score = score;
                unsigned new_hash = hash;
                for (int i = 0; i < visited.size(); i++) {
                    int idx_board = choosen_stages[i];
                    const auto& visite = visited[i];
                    const auto& player = players[i];
                    const auto uyx = player + dyx;
                    if (input.stage[idx_board][uyx.y][uyx.x] == 'x') goto brcon;
                    if (input.stage[idx_board][uyx.y][uyx.x] == 'o') {
                        if (!visite.Get(uyx)) {
                            new_score++;
                            new_hash ^= hashes[i << 12 | uyx.y << 6 | uyx.x];
                        }
                    }
                    if (input.stage[idx_board][uyx.y][uyx.x] != '#') {
                        new_hash ^= hashes[1 << 15 | i << 12 | uyx.y << 6 | uyx.x]
                                  ^ hashes[1 << 15 | i << 12 | player.y << 6 | player.x];
                    }
                }
                res.push({ new_score, Action{ d }, new_hash });
            brcon:;
            }
        }
        inline void Do(const Action& action) {
            const auto& dyx = dyxs[action.d];
            for (int i = 0; i < visited.size(); i++) {
                int idx_board = choosen_stages[i];
                auto& visite = visited[i];
                auto& player = players[i];
                const auto uyx = player + dyx;
                if (input.stage[idx_board][uyx.y][uyx.x] != '#') {
                    hash ^= hashes[1 << 15 | i << 12 | uyx.y << 6 | uyx.x]
                          ^ hashes[1 << 15 | i << 12 | player.y << 6 | player.x];
                    player = uyx;
                }
                if (input.stage[idx_board][uyx.y][uyx.x] == 'o') {
                    if (!visite.Get(uyx)) {
                        score++;
                        visite.Set(uyx, true);
                        hash ^= hashes[i << 12 | uyx.y << 6 | uyx.x];
                    }
                }
            }
            command.Set(turn, action.d);
            turn++;
            if (turn == 2500) termination = true;
        }
        inline void PrintAnswer() const {
            for (int i = 0; i < 8; i++) {
                cout << choosen_stages[i] << " \n"[i == 7];
            }
            command.Print();
        }
    };


    void Solve() {
        input.Read();
        choose_stages();
        Random rng(42);
        for (auto&& h : hashes) h = (unsigned)rng.next();
        static State initial_state;
        static BeamSearch<State> beam_search(initial_state);
        beam_search.Search();
        beam_search.BestState().PrintAnswer();
        cout << beam_search.BestState().score << endl;
    }
}

int main() {
    RCO2018QualA::Solve();
}

