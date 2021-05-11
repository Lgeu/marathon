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
    Stack<Node, (int)3e6>* nodes;  // 追加したら動かさない・消さない  // 実体だとセグフォる…  // 確保に時間かかってる？
    Node* ptr_current_node;
    Stack<Node*, max_depth> path_to_redo;


    inline SearchTree(const State& initial_state) :
        current_state(initial_state),
        nodes(),
        ptr_current_node() {
        nodes = new Stack<Node, (int)3e6>(1);
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
        //ASSERT_RANGE(0, 0, path_to_redo.size());
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
struct ChokudaiSearch {
    // デフォルトは最大化

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

    inline ChokudaiSearch(const State& initial_state) :  // 終わるターン数が同じ場合はこれでいいけど…？ // state.act が終端状態を返すのでこれを使う(TODO)
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

struct BeamSearch {
};
