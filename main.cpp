//#include <unistd.h>
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
            if (turn < 30) {
                //cout << "GetNextStates" << " score=" << score << " turn=" << turn << endl;
            }
            for (char c = 0; c < 9; c++) {
                next_states.push({
                    (double)(-turn + score + queues[c].size()),  // このスコアは少し嘘だが…？
                    Action{c}
                    });
            }
            return score == input.N * input.N;
        }

        pair<Patch, ReversePatch> Do(const Action& action) {
            if (turn < 30) {//cout << "Do" << endl;
                for (auto& q : queues) {
                    //cout << q.size() << " ";
                }
                //cout << endl;
            }
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
            //cout << "Redo" << endl;
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
            //cout << "Undo" << endl;
            answer.pop();
            for (const auto& t : reverse_patch.done) {
                ASSERT_RANGE(t.first, 0, done.right);
                done[t.first] = t.second;
            }
            if (turn < 30) {//cout << "Undo-queues ";
              //for(const auto& cnt : reverse_patch.q_count) cout << cnt << " ";
              //cout << endl;
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

}  // namespace Chokudai005


int main() {
    static Chokudai005::Input input;
    input.read();
    static const Chokudai005::State state(input);
    //input.print();
    static ChokudaiSearch<Chokudai005::State, 10000> s(state);
    s.Search(2.4, 10000);
    //cout << "best score = " << s.best_state.score << " " << (s.best_state.ptr_node==nullptr) << endl;
    static auto stt = s.BestState();
    static auto path = s.BestStatePath();
    //cout << stt.score << " " << stt.turn << " " << endl;
    //cout << "n_nodes=" << s.tree.nodes->size() << endl;

    cout << path.size() - 1 << endl;
    for (auto it = path.begin() + 1; it != path.end(); it++) {
        cout << input.N / 2 + 1 << " " << input.N / 2 + 1 << " " << (int)it->color + 1 << endl;
    }
}