using namespace std;

template<class State, int max_depth>
struct SearchTree {
    using Action = typename State::Action;
    using Patch = typename State::Patch;
    using ReversePatch = typename State::ReversePatch;
    using NewStateInfo = typename State::NewStateInfo;

    struct Node {
        const Action action;  // 親ノードからこのノードに至る行動
        Patch patch;  // 親ノードからこのノードに至る行動
        ReversePatch reverse_patch;  // このノードから親ノードに戻す行動
        const int depth;
        Node* const ptr_parent;
        bool searched;  // Do をしたか
        inline Node& operator=(const Node& rhs) {
            this->~Node();
            new(this) Node(rhs);
            return *this;
        }
        inline Node() :
            action(),
            patch(),
            reverse_patch(),
            depth(0),
            ptr_parent(nullptr),
            searched(false) {}
        inline Node(  // Patch なし
            const Action& a_action,
            Node* const a_ptr_parent
        ) :
            action(a_action),
            patch(),
            reverse_patch(),
            depth(a_ptr_parent->depth + 1),
            ptr_parent(a_ptr_parent),
            searched(false) {}
        inline Node(  // 使わなさそう
            const Action& a_action,
            const Patch& a_patch,
            const ReversePatch& a_reverse_patch,
            Node* const a_ptr_parent
        ) :
            action(a_action),
            patch(a_patch),
            reverse_patch(a_reverse_patch),
            depth(a_ptr_parent->depth + 1),
            ptr_parent(a_ptr_parent),
            searched(true) {}
    };

    State current_state;
    Stack<Node, (int)1e6>* nodes;  // 追加したら動かさない・消さない  // 実体だとセグフォる…  // 確保に時間かかってる？
    Node* ptr_current_node;
    Stack<Node*, max_depth> path_to_redo;


    inline SearchTree(const State& initial_state) :
        current_state(initial_state),
        nodes(),
        ptr_current_node() {
        nodes = new Stack<Node, (int)1e6>(1);
        ptr_current_node = &(nodes->operator[](0));
    }
    inline ~SearchTree() {
        delete nodes;
    }

    inline Node* Root() {
        ASSERT_RANGE(0, 0, nodes->size());
        return &(nodes->operator[](0));
    }

    inline void ChangeState(Node* ptr_target_node) {
        if (ptr_target_node == ptr_current_node) return;
        const int depth_diff = ptr_target_node->depth - ptr_current_node->depth;

        if (depth_diff > 0) {  // 移動先の方が深い
            for (int i = 0; i < depth_diff; i++) {
                path_to_redo.push(ptr_target_node);
                ptr_target_node = ptr_target_node->ptr_parent;
            }
        }
        else {  // 移動前の方が深い
            for (int i = 0; i > depth_diff; i--) {
                current_state.Undo(ptr_current_node->reverse_patch);
                ptr_current_node = ptr_current_node->ptr_parent;
            }
        }
        while (ptr_current_node != ptr_target_node) {  // 共通祖先まで遡る
            path_to_redo.push(ptr_target_node);
            ptr_target_node = ptr_target_node->ptr_parent;
            ASSERT(ptr_target_node != nullptr, "hoge~~");
            current_state.Undo(ptr_current_node->reverse_patch);
            ptr_current_node = ptr_current_node->ptr_parent;
        }
        if (!path_to_redo.empty()) {
            ptr_current_node = path_to_redo[0];
            while (!path_to_redo.empty()) {
                Node* ptr = path_to_redo.pop();
                if (ptr->searched) current_state.Redo(ptr->patch);
                else {
                    auto patches = current_state.Do(ptr->action);
                    ptr->patch = patches.first;
                    ptr->reverse_patch = patches.second;
                }
            }
        }
    }

    inline Node* AddNode(const NewStateInfo& r) {
        nodes->emplace(
            r.action,
            ptr_current_node
        );
        return &nodes->back();
    }

    template<int max_n_next_states>
    inline bool GetNextStates(Node* const node, Stack<NewStateInfo, max_n_next_states>& next_states) {
        ChangeState(node);
        return current_state.GetNextStates(next_states);
    }

    inline Stack<Action, max_depth> GetPath() const {
        const Node* ptr_node = ptr_current_node;
        Stack<Action, max_depth> res;
        while (ptr_node->depth) {
            res.push(ptr_node->action);
            ptr_node = ptr_node->ptr_parent;
        }
        reverse(res.begin(), res.end());
        return res;
    }
};

template<class State, int max_n_turns>
struct ColunChokudaiSearch {
    // ・(状態 x 行動) にスコアを持つ
    // ・inplace に状態を変更
    // ・最大化
    // ・1 つ先の状態にのみ遷移

    struct Candidate {
        double score;
        typename SearchTree<State, max_n_turns>::Node* ptr_node;
        // 必要に応じてハッシュ？（ここにはいらないかも）
        // 必要に応じて他の情報？（ここにはいらないかも）

        inline Candidate(const double& a_score, typename SearchTree<State, max_n_turns>::Node* a_ptr_node) :
            score(a_score), ptr_node(a_ptr_node) {}

        inline bool operator<(const Candidate& rhs) const {
            return score < rhs.score;
        }
    };

    Stack<priority_queue<Candidate>, max_n_turns + 1> candidates;
    SearchTree<State, max_n_turns> tree;
    Stack<typename State::NewStateInfo, 10000> next_states;
    Candidate best_state;

    inline ColunChokudaiSearch(const State& initial_state) :
        candidates(max_n_turns + 1),
        tree(initial_state),
        best_state{ -1e300, nullptr }
    {
        candidates[0].push(Candidate{ 0.0, tree.Root() });
    }

    inline void Search(const double& time_limit, const int& n_turns) {
        const double t0 = time();
        while (time() - t0 < time_limit) {
            int chokudai_width = 1;
            for (int turn = 0; turn < n_turns; turn++) {
                if (chokudai_width == 0) break;
                for (int i = 0; i < chokudai_width; i++) {
                    if (candidates[turn].empty()) break;
                    //const Candidate best_candidate = candidates[turn].pop();
                    const Candidate best_candidate = candidates[turn].top();  candidates[turn].pop();
                    const bool termination = tree.GetNextStates(best_candidate.ptr_node, next_states);
                    for (const typename State::NewStateInfo& r : next_states) {
                        candidates[turn + 1].emplace(r.score, tree.AddNode(r));
                    }
                    if (termination) {
                        if (best_state.score < best_candidate.score) best_state = best_candidate;
                        chokudai_width--;
                    }
                    next_states.clear();  // これ tree 側でやるべきか…？
                }
            }
        }
    }

    inline State BestState() {
        tree.ChangeState(best_state.ptr_node);
        return tree.current_state;
    }

    Stack<typename State::Action, max_n_turns> BestStatePath() {
        tree.ChangeState(best_state.ptr_node);
        return tree.GetPath();
    }

};


template<class State>
struct BeamSearch {
    // ・状態にスコアを持つ
    // ・(状態 x 行動) を直接キューに溜め込む
    // ・最大化
    // ・1 つ先の状態にのみ遷移
    // ・ハッシュで類似状態除去しない
    struct Candidate {
        double score;
        State* ptr_parent_state;
        typename State::Action action;
        inline Candidate() : score(0.0), ptr_parent_state(nullptr), action() {}
        inline Candidate(const double& a_score, State* const a_ptr_parent_state, const typename State::Action a_action) :
            score(a_score), ptr_parent_state(a_ptr_parent_state), action(a_action) {}
        inline bool operator<(const Candidate& rhs) const {
            return score < rhs.score;
        }
        inline bool operator>(const Candidate& rhs) const {
            return score > rhs.score;
        }
    };
    Stack<Candidate, 200000> candidates;
    State best_state;
    Stack<typename State::NewStateInfo, 10000> next_states;
    array<Stack<State, 2300>, 2> parent_states;  // ビーム幅以上
    inline BeamSearch(const State& initial_state) :
        candidates(), best_state(initial_state), next_states(), parent_states()
    {
        best_state.score = -1e300;
        parent_states[0].push(initial_state);
    }
    inline void Search() {
        int beam_width = 2300;
        for (int depth = 0;; depth++) {
            if (beam_width == 0) break;
            for (int i = 0; i < parent_states[depth % 2].size(); i++) {
                auto& parent_state = parent_states[depth % 2][i];
                parent_state.GetNextStates(next_states);
                for (const auto& r : next_states) {
                    candidates.emplace(r.score, &parent_state, r.action);
                }
                next_states.clear();
            }
            if (candidates.size() == 0) break;
            if (beam_width < candidates.size()) {
                nth_element(candidates.begin(), candidates.begin() + beam_width, candidates.end(), std::greater<>());
                candidates.resize(beam_width);
            }
            parent_states[(depth + 1) % 2].resize(candidates.size());
            for (int i = 0; i < candidates.size(); i++) {
                auto& candidate = candidates[i];
                auto& state = parent_states[(depth + 1) % 2][i];
                state = *candidate.ptr_parent_state;
                state.Do(candidate.action);
                if (state.termination) {
                    if (best_state.score < state.score) best_state = state;
                    beam_width--;
                }
            }
            candidates.clear();
        }
    }
    inline State BestState() {
        return best_state;
    }
};

template<class State, int hash_table_size=26>
struct BeamSearchWithHash {
    // ・状態にスコアを持つ
    // ・(状態 x 行動) を直接キューに溜め込む
    // ・最大化
    // ・1 つ先の状態にのみ遷移
    // ・ハッシュで類似状態除去する
    // ・ハッシュが同じならスコアも同じ
    struct Candidate {
        double score;
        State* ptr_parent_state;
        typename State::Action action;
        inline Candidate() : score(0.0), ptr_parent_state(nullptr), action() {}
        inline Candidate(const double& a_score, State* const a_ptr_parent_state, const typename State::Action a_action) :
            score(a_score), ptr_parent_state(a_ptr_parent_state), action(a_action) {}
        inline bool operator<(const Candidate& rhs) const { return score < rhs.score; }
        inline bool operator>(const Candidate& rhs) const { return score > rhs.score; }
    };

    // ---------- variables ----------
    Stack<Candidate, 200000> candidates;
    State best_state;
    Stack<typename State::NewStateInfo, 10000> next_states;
    array<Stack<State, 1200>, 2> parent_states;  // ビーム幅以上
    bitset<1 << hash_table_size> candidates_contains;  // 2^26 bits == 8 MB

    // ---------- constructors ----------
    inline BeamSearchWithHash(const State& initial_state) :
        candidates(), best_state(initial_state), next_states(), parent_states(), candidates_contains()
    {
        best_state.score = -1e300;
        parent_states[0].push(initial_state);
    }

    // ---------- methods ----------
    inline void Search(int max_depth=INT_MAX) {
        int beam_width = 1200;
        for (int depth = 0; depth < max_depth; depth++) {  // 最終状態に beam_width 個到達しないと RE になる
            if (beam_width == 0) break;
            for (int i = 0; i < parent_states[depth % 2].size(); i++) {
                auto& parent_state = parent_states[depth % 2][i];
                parent_state.GetNextStates(next_states);
                for (const auto& r : next_states) {
                    if (!candidates_contains[r.hash & (1u << hash_table_size) - 1u]) {
                        candidates.emplace(r.score, &parent_state, r.action);
                        candidates_contains[r.hash & (1u << hash_table_size) - 1u] = true;
                    }
                }
                next_states.clear();
            }
            if (candidates.size() == 0) break;
            if (beam_width < candidates.size()) {
                nth_element(candidates.begin(), candidates.begin() + beam_width, candidates.end(), std::greater<>());
                candidates.resize(beam_width);
            }
            parent_states[(depth + 1) % 2].resize(candidates.size());
            for (int i = 0; i < candidates.size(); i++) {
                auto& candidate = candidates[i];
                auto& state = parent_states[(depth + 1) % 2][i];
                state = *candidate.ptr_parent_state;
                state.Do(candidate.action);
                if (state.termination) {
                    if (best_state.score < state.score) best_state = state;
                    beam_width--;
                }
            }
            
            candidates.clear();
            candidates_contains.reset();
        }
    }
    inline State BestState() {
        return best_state;
    }
};
