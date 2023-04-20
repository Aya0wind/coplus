#pragma once
#include <fmt/format.h>
#include <fmt/ranges.h>
template <class...> constexpr std::false_type always_false{};

template<class T>
using Iterator = std::iterator_traits<decltype(std::begin(std::declval<T>()))>;

template<class T,class node_ptr = std::add_pointer_t<std::remove_cvref<typename T::node_type>>>
concept TreeIterable = requires(T obj,node_ptr ptr){
                           {obj.get_root()} ->std::convertible_to<node_ptr>;
                           {ptr->left} ->std::convertible_to<node_ptr>;
                           {ptr->right} ->std::convertible_to<node_ptr>;
                       };
template<class T>
concept ChainIterable = std::same_as<typename Iterator<T>::iterator_category,std::forward_iterator_tag>;

template<class T>
concept RandomIterable = std::same_as<typename Iterator<T>::iterator_category,std::random_access_iterator_tag>;

template <class T>
concept IterableContainer = TreeIterable<T>||ChainIterable<T>;//|| RandomIterable<T>;


template <TreeIterable T>
struct tree_utils{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(T&& c, FormatContext& ctx) const {
        auto root = c.get_root();
        auto buffer = print_to_buffer(root);
        fmt::format_to(ctx.out(),"{}",fmt::join(c, '\n'));
    }
private:
    using node_ptr = std::add_pointer_t<std::remove_cvref<typename T::node_type>>;

    static std::pair<int,int> get_width_and_height(node_ptr root){
        auto deep = tree_deep(root);
        return {4*(deep-1)+5,2*(deep-1)+1};
    }
    static std::vector<std::string> print_to_buffer(node_ptr root){
        auto [width,height] = get_width_and_height(root);
        auto result = std::vector<std::string>(height,std::string(width));
        return result;
    }


    static auto tree_deep(node_ptr root){
        if(!root)
            return 0;
        return std::max(tree_deep(root->left),tree_deep(root->right))+1;
    }

};

//template <IsPair PairT>
//struct fmt::formatter<PairT>{
//    // Parses format specifications of the form ['f' | 'e'].
//    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
//        return ctx.begin();
//    }
//    template <typename FormatContext>
//    auto format(PairT&& c, FormatContext& ctx) const {
//        fmt::format_to(ctx.out(), '{');
//        fmt::format_to(ctx.out(), "{}",fmt::join(c, ", "));
//        fmt::format_to(ctx.out(),'}');
//    }
//};
template <class T>
struct list_view{
    T container;
    list_view(const T& container):container(container){}
};

template <class T>
struct fmt::formatter<list_view<T>>{

    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const list_view<T>& c, FormatContext& ctx) const {
        if constexpr (ChainIterable<T>){
            fmt::format_to(ctx.out(),"{}",fmt::join(c.container, "->"));
            return fmt::format_to(ctx.out(),"{}","->null");
        }else if constexpr (RandomIterable<T>){
            fmt::format_to(ctx.out(), "{}",'[');
            fmt::format_to(ctx.out(),"{}",fmt::join(c.container, ", "));
            return fmt::format_to(ctx.out(), "{}",']');
        }else{
            static_assert(always_false<T>,"type cannot be formatted");
        }
    }
};

#include <cmath>
#include <concepts>
#include <optional>
#include <queue>
#include <string>

namespace tools {
struct ListNode {
    int val;
    ListNode* next;
    explicit ListNode(int x)
        : val(x)
        , next(nullptr)
    {
    }
};



class LinkedList
{
public:
    using value_type = int;
    using reference = const int&;
    using const_reference = const int&;
    using size_type = size_t;

    LinkedList(const std::initializer_list<int>& list)
        : listSize(list.size())
    {
        if (list.size() == 0) {
            this->head = nullptr;
            return;
        }
        this->head = new ListNode(*list.begin());
        ListNode* curNode = this->head;
        for (auto&& element : list) {
            curNode->next = new ListNode(element);
            curNode = curNode->next;
        }
    }

    LinkedList(const LinkedList& other)
        : listSize(other.listSize)
        , head(nullptr)
    {
        if (other.head) {
            auto otherHead = other.head;
            this->head = new ListNode(otherHead->val);
            auto curHead = this->head;
            while (otherHead->next) {
                curHead->next = new ListNode(otherHead->next->val);
                curHead = curHead->next;
                otherHead = otherHead->next;
            }
        }
    }

    LinkedList& operator=(LinkedList&& other) noexcept
    {
        this->head = other.head;
        this->listSize = other.listSize;
        return *this;
    }

    LinkedList(LinkedList&& other) noexcept
        : listSize(other.listSize)
        , head(other.takeOwnedHead())
    {
        other.head = nullptr;
    }

    explicit LinkedList(ListNode* raw)
    {
        size_t newSize = countListSize(raw);
        this->head = raw;
        this->listSize = newSize;
    }

    void push_front(int val)
    {
        if (this->head) {
            auto curHead = this->head;
            while (curHead->next) {
                curHead = curHead->next;
            }
            curHead->next = new ListNode(val);
        } else {
            head = new ListNode(val);
        }
    }

    [[nodiscard]] size_t size() const
    {
        return this->listSize;
    }

    void push_back(int val)
    {
        if (head) {
            auto new_head = new ListNode(val);
            new_head->next = head;
            head = new_head;
        } else {
            head = new ListNode(val);
        }
    }

    ~LinkedList()
    {
        while (head) {
            ListNode* delNode = head;
            head = head->next;
            delete delNode;
        }
    }

    ListNode* takeOwnedHead()
    {
        auto curHead = this->head;
        this->head = nullptr;
        return curHead;
    }

private:
    ListNode* head;
    size_t listSize;

    friend std::string to_string(const LinkedList& list);

    static size_t countListSize(ListNode* head)
    {
        size_t counter = 0;
        while (head) {
            counter += 1;
            head = head->next;
        }
        return counter;
    }
};

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    explicit TreeNode(int x)
        : val(x)
        , left(nullptr)
        , right(nullptr)
    {
    }
};

class BinaryTree
{
private:
    TreeNode* treeRootNode;

    void freeNode(TreeNode* root)
    {
        if (root == nullptr) {
            return;
        }
        freeNode(root->left);
        freeNode(root->right);
        delete root;
    };

    static std::vector<std::optional<int>> extractIntArray(std::string str)
    {
        std::vector<std::optional<int>> intList;
        str = str.substr(1, str.size() - 2);
        str += ',';
        for (int pos = 0; pos < str.size(); pos += 1) {
            int tailPos = static_cast<int>(str.find(',', pos));
            std::string intStr = str.substr(pos, tailPos - pos);

            if (intStr == "null") {
                intList.emplace_back(std::nullopt);
            } else if (intStr.empty()) {
                continue;
            } else {
                intList.emplace_back(stoi(intStr));
            }
            pos = tailPos;
        }
        return intList;
    }

    void setBinTreeByInt(std::vector<std::optional<int>>& list)
    {
        std::queue<TreeNode*> q;
        freeNode(this->treeRootNode);
        if (list.empty() || list[ 0 ] == std::nullopt) {
            return;
        }
        treeRootNode = new TreeNode(list[ 0 ].value());
        q.push(treeRootNode);
        for (int i = 1; i < list.size(); i++) {
            TreeNode* parent = q.front();
            if (list[ i ] != std::nullopt) {
                auto* newNode = new TreeNode(list[ i ].value());
                if ((i % 2) != 0) {
                    parent->left = newNode;
                } else {
                    parent->right = newNode;
                }
                q.push(newNode);
            }
            if (i % 2 == 0) {
                q.pop();
            }
        }
    };

    int treeHeight(TreeNode* root) const
    {
        if (root == nullptr) {
            return 0;
        }
        return std::max(treeHeight(root->left), treeHeight(root->right)) + 1;
    }

public:
    BinaryTree()
        : treeRootNode(nullptr){};

    explicit BinaryTree(const std::string& str)
        : treeRootNode(nullptr)
    {
        setBinTree(str);
    };
    ~BinaryTree()
    {
        freeNode(treeRootNode);
    };

    void setBinTree(const std::string& str)
    {
        std::vector intArray = extractIntArray(str);
        setBinTreeByInt(intArray);
    }

    TreeNode* getRoot()
    {
        return treeRootNode;
    }

    [[nodiscard, maybe_unused]] TreeNode* getInnerRoot()
    {
        return treeRootNode;
    }

    explicit operator TreeNode*()
    {
        return this->treeRootNode;
    }
    friend std::vector<std::optional<int>> flattenBinTree(const BinaryTree& tree);
};

[[nodiscard]]inline std::vector<std::optional<int>> flattenBinTree(const BinaryTree& tree)
{
    std::vector<std::optional<int>> flatIntList;
    std::queue<TreeNode*> q;
    q.push(tree.treeRootNode);

    if (tree.treeRootNode == nullptr) {
        return flatIntList;
    }

    while (!q.empty()) {
        TreeNode* ptn = q.front();
        if (ptn == nullptr) {
            flatIntList.emplace_back(std::nullopt);
        } else {
            flatIntList.emplace_back(ptn->val);
            q.push(ptn->left);
            q.push(ptn->right);
        }
        q.pop();
    }
    while (flatIntList.back() == std::nullopt) {
        flatIntList.pop_back();
    }
    return flatIntList;
}

inline std::vector<std::string> convertStrList(std::vector<std::optional<int>>& flatIntList)
{
    std::vector<std::string> flatStrList;
    for (auto var : flatIntList) {
        if (var == std::nullopt) {
            flatStrList.emplace_back("null");
        } else {
            flatStrList.push_back(std::to_string(var.value()));
        }
    }
    return flatStrList;
}
}  // namespace tools