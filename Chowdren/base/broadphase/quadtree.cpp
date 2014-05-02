#include "broadphase/quadtree.h"
#include "mathcommon.h"
#include <algorithm>
#include "common.h"

// QuadTreeNode

QuadTreeNode::QuadTreeNode()
: children(0), items(NULL)
{
}

QuadTreeNode::~QuadTreeNode()
{
    if (level != MAX_TREE_LEVEL)
        return;
    if (items == NULL)
        return;
    delete items;
}

void QuadTreeNode::add(unsigned int item, int v[4])
{
    children++;
    if (level == MAX_TREE_LEVEL) {
        ProxyItems & v = get_items();
        v.push_back(item);
        return;
    }
    if (collides_tree(v, a))
        a->add(item, v);
    if (collides_tree(v, b))
        b->add(item, v);
    if (collides_tree(v, c))
        c->add(item, v);
    if (collides_tree(v, d))
        d->add(item, v);
}

void QuadTreeNode::remove(unsigned int item)
{
    if (children == 0)
        return;
    if (level == MAX_TREE_LEVEL) {
        ProxyItems & v = get_items();
        v.erase(std::remove(v.begin(), v.end(), item), v.end());
    } else {
        a->remove(item);
        b->remove(item);
        c->remove(item);
        d->remove(item);
    }
}

ProxyItems & QuadTreeNode::get_items()
{
    if (items == NULL)
        items = new ProxyItems;
    return *items;
}

void QuadTreeNode::clear()
{
    children = 0;
    if (level == MAX_TREE_LEVEL) {
        get_items().clear();
        return;
    }
    a->clear();
    b->clear();
    c->clear();
    d->clear();
}

// QuadTree

QuadTree::QuadTree(int x1, int y1, int x2, int y2)
: QuadTreeNode(), query_id(0)
{
    init(x1, y1, x2, y2);
}

QuadTree::QuadTree()
: QuadTreeNode(), query_id(0)
{
    init(0, 0,
         global_manager->frame->width, global_manager->frame->height);
}

void QuadTree::init(int x1, int y1, int x2, int y2)
{
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    level = 0;

    std::vector<QuadTreeNode*> stack;
    stack.push_back(this);

    current_node = 0;

    while (!stack.empty()) {
        QuadTreeNode * node = stack.back();
        stack.pop_back();
        x1 = node->x1;
        y1 = node->y1;
        x2 = node->x2;
        y2 = node->y2;
        int xx = (x1 + x2) / 2;
        int yy = (y1 + y2) / 2;
        int new_level = node->level+1;

        node->a = create_node(xx, y1, x2, yy, new_level);
        node->b = create_node(x1, y1, xx, yy, new_level);
        node->c = create_node(x1, yy, xx, y2, new_level);
        node->d = create_node(xx, yy, x2, y2, new_level);

        if (new_level == MAX_TREE_LEVEL)
            continue;

        stack.push_back(node->a);
        stack.push_back(node->b);
        stack.push_back(node->c);
        stack.push_back(node->d);
    }
}

QuadTreeNode * QuadTree::create_node(int x1, int y1, int x2, int y2, int level)
{
    QuadTreeNode * node = &nodes[current_node];
    current_node++;
    node->x1 = x1;
    node->y1 = y1;
    node->x2 = x2;
    node->y2 = y2;
    node->level = level;
    return node;
}

unsigned int QuadTree::add(void * data, int v[4])
{
    int vv[4] = {v[0], v[1], v[2], v[3]};

    intersect(vv[0], vv[1], vv[2], vv[3],
              this->x1, this->y1, this->x2, this->y2,
              vv[0], vv[1], vv[2], vv[3]);

    QuadTreeItems::iterator it;
    int index = 0;
    for (it = store.begin(); it != store.end(); it++) {
        QuadTreeItem & item = *it;
        if (item.data == NULL) {
            item.data = data;
            QuadTreeNode::add(index, vv);
            return index;
        }
        index++;
    }

    index = store.size();
    store.resize(index + 1);
    QuadTreeItem & item = store[index];
    item.data = data;
    item.mark = query_id;
    QuadTreeNode::add(index, vv);
    return index;
}

void QuadTree::move(unsigned int item, int v[4])
{
    // XXX this is slow, we can do better
    QuadTreeNode::remove(item);
    int vv[4] = {v[0], v[1], v[2], v[3]};

    intersect(vv[0], vv[1], vv[2], vv[3],
              this->x1, this->y1, this->x2, this->y2,
              vv[0], vv[1], vv[2], vv[3]);
    QuadTreeNode::add(item, vv);
}

void QuadTree::remove(unsigned int item)
{
    store[item].data = NULL;
    QuadTreeNode::remove(item);
}
