/*****************************************************************************\
 *  Copyright (c) Athena Dev Teams - Licensed under GNU GPL                  *
 *  For more information, see LICENCE in the main folder                     *
 *                                                                           *
 *  This file is separated in five sections:                                 *
 *  (1) Private typedefs, enums, structures, defines and gblobal variables   *
 *  (2) Private functions                                                    *
 *  (3) Protected functions used internally                                  *
 *  (4) Protected functions used in the interface of the database            *
 *  (5) Public functions                                                     *
 *                                                                           *
 *  The databases are structured as a hashtable of RED-BLACK trees.          *
 *                                                                           *
 *  <B>Properties of the RED-BLACK trees being used:</B>                     *
 *  1. The value of any node is greater than the value of its left child and *
 *     less than the value of its right child.                               *
 *  2. Every node is colored either RED or BLACK.                            *
 *  3. Every red node that is not a leaf has only black children.            *
 *  4. Every path from the root to a leaf contains the same number of black  *
 *     nodes.                                                                *
 *  5. The root node is black.                                               *
 *  An <code>n</code> node in a RED-BLACK tree has the property that its     *
 *  height is <code>O(lg(n))</code>.                                         *
 *  Another important property is that after adding a node to a RED-BLACK    *
 *  tree, the tree can be readjusted in <code>O(lg(n))</code> time.          *
 *  Similarly, after deleting a node from a RED-BLACK tree, the tree can be  *
 *  readjusted in <code>O(lg(n))</code> time.                                *
 *  {@link http://www.cs.mcgill.ca/~cs251/OldCourses/1997/topic18/}          *
 *                                                                           *
 *  <B>How to add new database types:</B>                                    *
 *  1. Add the identifier of the new database type to the enum DBType        *
 *  2. If not already there, add the data type of the key to the union DBKey *
 *  3. If the key can be considered NULL, update the function db_is_key_null *
 *  4. If the key can be duplicated, update the functions db_dup_key and     *
 *     db_dup_key_free                                                       *
 *  5. Create a comparator and update the function db_default_cmp            *
 *  6. Create a hasher and update the function db_default_hash               *
 *  7. If the new database type requires or does not support some options,   *
 *     update the function db_fix_options                                    *
 *                                                                           *
 *  TODO:                                                                    *
 *  - create test cases to test the database system thoroughly               *
 *  - make data an enumeration                                               *
 *  - finish this header describing the database system                      *
 *  - create custom database allocator                                       *
 *  - make the system thread friendly                                        *
 *  - change the structure of the database to T-Trees                        *
 *  - create a db that organizes itself by splaying                          *
 *                                                                           *
 *  HISTORY:                                                                 *
 *    2.1 (Athena build #???#) - Portability fix                             *
 *      - Fixed the portability of casting to union and added the functions  *
 *        {@link DBInterface#ensure(DBInterface,DBKey,DBCreateData,...)} and *
 *        {@link DBInterface#clear(DBInterface,DBApply,...)}.                *
 *    2.0 (Athena build 4859) - Transition version                           *
 *      - Almost everything recoded with a strategy similar to objects,      *
 *        database structure is maintained.                                  *
 *    1.0 (up to Athena build 4706)                                          *
 *      - Previous database system.                                          *
 *                                                                           *
 * @version 2.1 (Athena build #???#) - Portability fix                       *
 * @author (Athena build 4859) Flavio @ Amazon Project                       *
 * @author (up to Athena build 4706) Athena Dev Teams                        *
 * @encoding US-ASCII                                                        *
 * @see common#db.h                                                          *
\*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "../common/mmo.h"
#include "../common/utils.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/ers.h"

/*****************************************************************************\
 *  (1) Private typedefs, enums, structures, defines and global variables of *
 *  the database system.                                                     *
 *  DB_ENABLE_STATS - Define to enable database statistics.                  *
 *  HASH_SIZE       - Define with the size of the hashtable.                 *
 *  DBNColor        - Enumeration of colors of the nodes.                    *
 *  DBNode          - Structure of a node in RED-BLACK trees.                *
 *  struct db_free  - Structure that holds a deleted node to be freed.       *
 *  Database        - Struture of the database.                              *
 *  stats           - Statistics about the database system.                  *
\*****************************************************************************/

/**
 * If defined statistics about database nodes, database creating/destruction 
 * and function usage are keept and displayed when finalizing the database
 * system.
 * WARNING: This adds overhead to every database operation (not shure how much).
 * @private
 * @see #DBStats
 * @see #stats
 * @see #db_final(void)
 */
//#define DB_ENABLE_STATS

/**
 * Size of the hashtable in the database.
 * @private
 * @see Database#ht
 */
#define HASH_SIZE (256+27)

/**
 * A node in a RED-BLACK tree of the database.
 * @param parent Parent node
 * @param left Left child node
 * @param right Right child node
 * @param key Key of this database entry
 * @param data Data of this database entry
 * @param deleted If the node is deleted
 * @param color Color of the node
 * @private
 * @see Database#ht
 */
typedef struct dbn {
	// Tree structure
	struct dbn *parent;
	struct dbn *left;
	struct dbn *right;
	// Node data
	DBKey key;
	void *data;
	// Other
	enum {RED, BLACK} color;
	unsigned deleted : 1;
} *DBNode;

/**
 * Structure that holds a deleted node.
 * @param node Deleted node
 * @param root Address to the root of the tree
 * @private
 * @see Database#free_list
 */
struct db_free {
	DBNode node;
	DBNode *root;
};

/**
 * Complete database structure.
 * @param dbi Interface of the database
 * @param alloc_file File where the database was allocated
 * @param alloc_line Line in the file where the database was allocated
 * @param free_list Array of deleted nodes to be freed
 * @param free_count Number of deleted nodes in free_list
 * @param free_max Current maximum capacity of free_list
 * @param free_lock Lock for freeing the nodes
 * @param nodes Manager of reusable tree nodes
 * @param cmp Comparator of the database
 * @param hash Hasher of the database
 * @param release Releaser of the database
 * @param ht Hashtable of RED-BLACK trees
 * @param type Type of the database
 * @param options Options of the database
 * @param item_count Number of items in the database
 * @param maxlen Maximum length of strings in DB_STRING and DB_ISTRING databases
 * @param global_lock Global lock of the database
 * @private
 * @see common\db.h#DBInterface
 * @see #HASH_SIZE
 * @see #DBNode
 * @see #struct db_free
 * @see common\db.h#DBComparator(void *,void *)
 * @see common\db.h#DBHasher(void *)
 * @see common\db.h#DBReleaser(void *,void *,DBRelease)
 * @see common\db.h#DBOptions
 * @see common\db.h#DBType
 * @see #db_alloc(const char *,int,DBOptions,DBType,...)
 */
typedef struct db {
	// Database interface
	struct dbt dbi;
	// File and line of allocation
	const char *alloc_file;
	int alloc_line;
	// Lock system
	struct db_free *free_list;
	unsigned int free_count;
	unsigned int free_max;
	unsigned int free_lock;
	// Other
	ERInterface nodes;
	DBComparator cmp;
	DBHasher hash;
	DBReleaser release;
	DBNode ht[HASH_SIZE];
	DBType type;
	DBOptions options;
	unsigned int item_count;
	unsigned short maxlen;
	unsigned global_lock : 1;
} *Database;

#ifdef DB_ENABLE_STATS
/**
 * Structure with what is counted when the database estatistics are enabled.
 * @private
 * @see #DB_ENABLE_STATS
 * @see #stats
 */
static struct {
	// Node alloc/free
	unsigned int db_node_alloc;
	unsigned int db_node_free;
	// Database creating/destruction counters
	unsigned int db_int_alloc;
	unsigned int db_uint_alloc;
	unsigned int db_string_alloc;
	unsigned int db_istring_alloc;
	unsigned int db_int_destroy;
	unsigned int db_uint_destroy;
	unsigned int db_string_destroy;
	unsigned int db_istring_destroy;
	// Function usage counters
	unsigned int db_rotate_left;
	unsigned int db_rotate_right;
	unsigned int db_rebalance;
	unsigned int db_rebalance_erase;
	unsigned int db_is_key_null;
	unsigned int db_dup_key;
	unsigned int db_dup_key_free;
	unsigned int db_free_add;
	unsigned int db_free_remove;
	unsigned int db_free_lock;
	unsigned int db_free_unlock;
	unsigned int db_int_cmp;
	unsigned int db_uint_cmp;
	unsigned int db_string_cmp;
	unsigned int db_istring_cmp;
	unsigned int db_int_hash;
	unsigned int db_uint_hash;
	unsigned int db_string_hash;
	unsigned int db_istring_hash;
	unsigned int db_release_nothing;
	unsigned int db_release_key;
	unsigned int db_release_data;
	unsigned int db_release_both;
	unsigned int db_get;
	unsigned int db_getall;
	unsigned int db_vgetall;
	unsigned int db_ensure;
	unsigned int db_vensure;
	unsigned int db_put;
	unsigned int db_remove;
	unsigned int db_foreach;
	unsigned int db_vforeach;
	unsigned int db_clear;
	unsigned int db_vclear;
	unsigned int db_destroy;
	unsigned int db_vdestroy;
	unsigned int db_size;
	unsigned int db_type;
	unsigned int db_options;
	unsigned int db_fix_options;
	unsigned int db_default_cmp;
	unsigned int db_default_hash;
	unsigned int db_default_release;
	unsigned int db_custom_release;
	unsigned int db_alloc;
	unsigned int db_i2key;
	unsigned int db_ui2key;
	unsigned int db_str2key;
	unsigned int db_init;
	unsigned int db_final;
} stats = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};
#endif /* DB_ENABLE_STATS */

/*****************************************************************************\
 *  (2) Section of private functions used by the database system.            *
 *  db_rotate_left     - Rotate a tree node to the left.                     *
 *  db_rotate_right    - Rotate a tree node to the right.                    *
 *  db_rebalance       - Rebalance the tree.                                 *
 *  db_rebalance_erase - Rebalance the tree after a BLACK node was erased.   *
 *  db_is_key_null     - Returns not 0 if the key is considered NULL.        *
 *  db_dup_key         - Duplicate a key for internal use.                   *
 *  db_dup_key_free    - Free the duplicated key.                            *
 *  db_free_add        - Add a node to the free_list of a database.          *
 *  db_free_remove     - Remove a node from the free_list of a database.     *
 *  db_free_lock       - Increment the free_lock of a database.              *
 *  db_free_unlock     - Decrement the free_lock of a database.              *
 *         If it was the last lock, frees the nodes in free_list.            *
 *         NOTE: Keeps the database trees balanced.                          *
\*****************************************************************************/

/**
 * Rotate a node to the left.
 * @param node Node to be rotated
 * @param root Pointer to the root of the tree
 * @private
 * @see #db_rebalance(DBNode,DBNode *)
 * @see #db_rebalance_erase(DBNode,DBNode *)
 */
static void db_rotate_left(DBNode node, DBNode *root)
{
	DBNode y = node->right;

#ifdef DB_ENABLE_STATS
	if (stats.db_rotate_left != (unsigned int)~0) stats.db_rotate_left++;
#endif /* DB_ENABLE_STATS */
	// put the left of y at the right of node
	node->right = y->left;
	if (y->left)
		y->left->parent = node;
	y->parent = node->parent;
	// link y and node's parent
	if (node == *root) {
		*root = y; // node was root
	} else if (node == node->parent->left) {
		node->parent->left = y; // node was at the left
	} else {
		node->parent->right = y; // node was at the right
	}
	// put node at the left of y
	y->left = node;
	node->parent = y;
}

/**
 * Rotate a node to the right
 * @param node Node to be rotated
 * @param root Pointer to the root of the tree
 * @private
 * @see #db_rebalance(DBNode,DBNode *)
 * @see #db_rebalance_erase(DBNode,DBNode *)
 */
static void db_rotate_right(DBNode node, DBNode *root)
{
	DBNode y = node->left;

#ifdef DB_ENABLE_STATS
	if (stats.db_rotate_right != (unsigned int)~0) stats.db_rotate_right++;
#endif /* DB_ENABLE_STATS */
	// put the right of y at the left of node
	node->left = y->right;
	if (y->right != 0)
		y->right->parent = node;
	y->parent = node->parent;
	// link y and node's parent
	if (node == *root) {
		*root = y; // node was root
	} else if (node == node->parent->right) {
		node->parent->right = y; // node was at the right
	} else {
		node->parent->left = y; // node was at the left
	}
	// put node at the right of y
	y->right = node;
	node->parent = y;
}

/**
 * Rebalance the RED-BLACK tree.
 * Called when the node and it's parent are both RED.
 * @param node Node to be rebalanced
 * @param root Pointer to the root of the tree
 * @private
 * @see #db_rotate_left(DBNode,DBNode *)
 * @see #db_rotate_right(DBNode,DBNode *)
 * @see #db_put(DBInterface,DBKey,void *)
 */
static void db_rebalance(DBNode node, DBNode *root)
{
	DBNode y;

#ifdef DB_ENABLE_STATS
	if (stats.db_rebalance != (unsigned int)~0) stats.db_rebalance++;
#endif /* DB_ENABLE_STATS */
	// Restore the RED-BLACK properties
	node->color = RED;
	while (node != *root && node->parent->color == RED) {
		if (node->parent == node->parent->parent->left) {
			// If node's parent is a left, y is node's right 'uncle'
			y = node->parent->parent->right;
			if (y && y->color == RED) { // case 1
				// change the colors and move up the tree
				node->parent->color = BLACK;
				y->color = BLACK;
				node->parent->parent->color = RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->right) { // case 2
					// move up and rotate
					node = node->parent;
					db_rotate_left(node, root);
				}
				// case 3
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				db_rotate_right(node->parent->parent, root);
			}
		} else {
			// If node's parent is a right, y is node's left 'uncle'
			y = node->parent->parent->left;
			if (y && y->color == RED) { // case 1
				// change the colors and move up the tree
				node->parent->color = BLACK;
				y->color = BLACK;
				node->parent->parent->color = RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->left) { // case 2
					// move up and rotate
					node = node->parent;
					db_rotate_right(node, root);
				}
				// case 3
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				db_rotate_left(node->parent->parent, root);
			}
		}
	}
	(*root)->color = BLACK; // the root can and should always be black
}

/**
 * Erase a node from the RED-BLACK tree, keeping the tree balanced.
 * @param node Node to be erased from the tree
 * @param root Root of the tree
 * @private
 * @see #db_rotate_left(DBNode,DBNode *)
 * @see #db_rotate_right(DBNode,DBNode *)
 * @see #db_free_unlock(Database)
 */
static void db_rebalance_erase(DBNode node, DBNode *root)
{
	DBNode y = node;
	DBNode x = NULL;
	DBNode x_parent = NULL;
	DBNode w;

#ifdef DB_ENABLE_STATS
	if (stats.db_rebalance_erase != (unsigned int)~0) stats.db_rebalance_erase++;
#endif /* DB_ENABLE_STATS */
	// Select where to change the tree
	if (y->left == NULL) { // no left
		x = y->right;
	} else if (y->right == NULL) { // no right
		x = y->left;
	} else { // both exist, go to the leftmost node of the right sub-tree
		y = y->right;
		while (y->left != NULL)
			y = y->left;
		x = y->right;
	}

	// Remove the node from the tree
	if (y != node) { // both childs existed
		// put the left of 'node' in the left of 'y'
		node->left->parent = y;
		y->left = node->left;

		// 'y' is not the direct child of 'node'
		if (y != node->right) {
			// put 'x' in the old position of 'y'
			x_parent = y->parent;
			if (x) x->parent = y->parent;
			y->parent->left = x;
			// put the right of 'node' in 'y' 
			y->right = node->right;
			node->right->parent = y;
		// 'y' is a direct child of 'node'
		} else {
			x_parent = y;
		}

		// link 'y' and the parent of 'node'
		if (*root == node) {
			*root = y; // 'node' was the root
		} else if (node->parent->left == node) {
			node->parent->left = y; // 'node' was at the left
		} else {
			node->parent->right = y; // 'node' was at the right
		}
		y->parent = node->parent;
		// switch colors
		{
			int tmp = y->color;
			y->color = node->color;
			node->color = tmp;
		}
		y = node;
	} else { // one child did not exist
		// put x in node's position
		x_parent = y->parent;
		if (x) x->parent = y->parent;
		// link x and node's parent
		if (*root == node) {
			*root = x; // node was the root
		} else if (node->parent->left == node) {
			node->parent->left = x; // node was at the left
		} else {
			node->parent->right = x;  // node was at the right
		}
	}

	// Restore the RED-BLACK properties
	if (y->color != RED) {
		while (x != *root && (x == NULL || x->color == BLACK)) {
			if (x == x_parent->left) {
				w = x_parent->right;
				if (w->color == RED) {
					w->color = BLACK;
					x_parent->color = RED;
					db_rotate_left(x_parent, root);
					w = x_parent->right;
				}
				if ((w->left == NULL || w->left->color == BLACK) &&
					(w->right == NULL || w->right->color == BLACK)) {
					w->color = RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if (w->right == NULL ||	w->right->color == BLACK) {
						if (w->left) w->left->color = BLACK;
						w->color = RED;
						db_rotate_right(w, root);
						w = x_parent->right;
					}
					w->color = x_parent->color;
					x_parent->color = BLACK;
					if (w->right) w->right->color = BLACK;
					db_rotate_left(x_parent, root);
					break;
				}
			} else {
				w = x_parent->left;
				if (w->color == RED) {
					w->color = BLACK;
					x_parent->color = RED;
					db_rotate_right(x_parent, root);
					w = x_parent->left;
				}
				if ((w->right == NULL || w->right->color == BLACK) &&
					(w->left == NULL || w->left->color == BLACK)) {
					w->color = RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if (w->left == NULL || w->left->color == BLACK) {
						if (w->right) w->right->color = BLACK;
						w->color = RED;
						db_rotate_left(w, root);
						w = x_parent->left;
					}
					w->color = x_parent->color;
					x_parent->color = BLACK;
					if (w->left) w->left->color = BLACK;
					db_rotate_right(x_parent, root);
					break;
				}
			}
		}
		if (x) x->color = BLACK;
	}
}

/**
 * Returns not 0 if the key is considerd to be NULL.
 * @param type Type of database
 * @param key Key being tested
 * @return not 0 if considered NULL, 0 otherwise
 * @private
 * @see common\db.h#DBType
 * @see common\db.h#DBKey
 * @see #db_get(DBInterface,DBKey)
 * @see #db_put(DBInterface,DBKey,void *)
 * @see #db_remove(DBInterface,DBKey)
 */
static int db_is_key_null(DBType type, DBKey key)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_is_key_null != (unsigned int)~0) stats.db_is_key_null++;
#endif /* DB_ENABLE_STATS */
	switch (type) {
		case DB_STRING:
		case DB_ISTRING:
			return (key.str == NULL);

		default: // Not a pointer
			return 0;
	}
}

/**
 * Duplicate the key used in the database.
 * @param db Database the key is being used in
 * @param key Key to be duplicated
 * @param Duplicated key
 * @private
 * @see #db_free_add(Database,DBNode,DBNode *)
 * @see #db_free_remove(Database,DBNode)
 * @see #db_put(DBInterface,DBKey,void *)
 * @see #db_dup_key_free(Database,DBKey)
 */
static DBKey db_dup_key(Database db, DBKey key)
{
	unsigned char *str;

#ifdef DB_ENABLE_STATS
	if (stats.db_dup_key != (unsigned int)~0) stats.db_dup_key++;
#endif /* DB_ENABLE_STATS */
	switch (db->type) {
		case DB_STRING:
		case DB_ISTRING:
			if (db->maxlen) {
				CREATE(str, unsigned char, db->maxlen +1);
				memcpy(str, key.str, db->maxlen);
				str[db->maxlen] = '\0';
				key.str = str;
			} else {
				key.str = (unsigned char *)aStrdup((const char *)key.str);
			}
			return key;

		default:
			return key;
	}
}

/**
 * Free a key duplicated by db_dup_key.
 * @param db Database the key is being used in
 * @param key Key to be freed
 * @private
 * @see #db_dup_key(Database,DBKey)
 */
static void db_dup_key_free(Database db, DBKey key)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_dup_key_free != (unsigned int)~0) stats.db_dup_key_free++;
#endif /* DB_ENABLE_STATS */
	switch (db->type) {
		case DB_STRING:
		case DB_ISTRING:
			aFree(key.str);
			return;

		default:
			return;
	}
}

/**
 * Add a node to the free_list of the database.
 * Marks the node as deleted.
 * If the key isn't duplicated, the key is duplicated and released.
 * @param db Target database
 * @param root Root of the tree from the node
 * @param node Target node
 * @private
 * @see #struct db_free
 * @see Database#free_list
 * @see Database#free_count
 * @see Database#free_max
 * @see #db_remove(DBInterface,DBKey)
 * @see #db_free_remove(Database,DBNode)
 */
static void db_free_add(Database db, DBNode node, DBNode *root)
{
	DBKey old_key;

#ifdef DB_ENABLE_STATS
	if (stats.db_free_add != (unsigned int)~0) stats.db_free_add++;
#endif /* DB_ENABLE_STATS */
	if (db->free_lock == (unsigned int)~0) {
		ShowFatalError("db_free_add: free_lock overflow\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
		exit(EXIT_FAILURE);
	}
	if (!(db->options&DB_OPT_DUP_KEY)) { // Make shure we have a key until the node is freed
		old_key = node->key;
		node->key = db_dup_key(db, node->key);
		db->release(old_key, node->data, DB_RELEASE_KEY);
	}
	if (db->free_count == db->free_max) { // No more space, expand free_list
		db->free_max = (db->free_max<<2) +3; // = db->free_max*4 +3
		if (db->free_max <= db->free_count) {
			if (db->free_count == (unsigned int)~0) {
				ShowFatalError("db_free_add: free_count overflow\n"
						"Database allocated at %s:%d\n",
						db->alloc_file, db->alloc_line);
				exit(EXIT_FAILURE);
			}
			db->free_max = (unsigned int)~0;
		}
		RECREATE(db->free_list, struct db_free, db->free_max);
	}
	node->deleted = 1;
	db->free_list[db->free_count].node = node;
	db->free_list[db->free_count].root = root;
	db->free_count++;
	db->item_count--;
}

/**
 * Remove a node from the free_list of the database.
 * Marks the node as not deleted.
 * NOTE: Frees the duplicated key of the node.
 * @param db Target database
 * @param node Node being removed from free_list
 * @private
 * @see #struct db_free
 * @see Database#free_list
 * @see Database#free_count
 * @see #db_put(DBInterface,DBKey,void *)
 * @see #db_free_add(Database,DBNode *,DBNode)
 */
static void db_free_remove(Database db, DBNode node)
{
	unsigned int i;

#ifdef DB_ENABLE_STATS
	if (stats.db_free_remove != (unsigned int)~0) stats.db_free_remove++;
#endif /* DB_ENABLE_STATS */
	for (i = 0; i < db->free_count; i++) {
		if (db->free_list[i].node == node) {
			if (i < db->free_count -1) // copy the last item to where the removed one was
				memcpy(&db->free_list[i], &db->free_list[db->free_count -1], sizeof(struct db_free));
			db_dup_key_free(db, node->key);
			break;
		}
	}
	node->deleted = 0;
	if (i == db->free_count) {
		ShowWarning("db_free_remove: node was not found - database allocated at %s:%d\n", db->alloc_file, db->alloc_line);
	} else {
		db->free_count--;
	}
	db->item_count++;
}

/**
 * Increment the free_lock of the database.
 * @param db Target database
 * @private
 * @see Database#free_lock
 * @see #db_unlock(Database)
 */
static void db_free_lock(Database db)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_free_lock != (unsigned int)~0) stats.db_free_lock++;
#endif /* DB_ENABLE_STATS */
	if (db->free_lock == (unsigned int)~0) {
		ShowFatalError("db_free_lock: free_lock overflow\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
		exit(EXIT_FAILURE);
	}
	db->free_lock++;
}

/**
 * Decrement the free_lock of the database.
 * If it was the last lock, frees the nodes of the database.
 * Keeps the tree balanced.
 * NOTE: Frees the duplicated keys of the nodes
 * @param db Target database
 * @private
 * @see Database#free_lock
 * @see #db_free_dbn(DBNode)
 * @see #db_lock(Database)
 */
static void db_free_unlock(Database db)
{
	unsigned int i;

#ifdef DB_ENABLE_STATS
	if (stats.db_free_unlock != (unsigned int)~0) stats.db_free_unlock++;
#endif /* DB_ENABLE_STATS */
	if (db->free_lock == 0) {
		ShowWarning("db_free_unlock: free_lock was already 0\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
	} else {
		db->free_lock--;
	}
	if (db->free_lock)
		return; // Not last lock

	for (i = 0; i < db->free_count ; i++) {
		db_rebalance_erase(db->free_list[i].node, db->free_list[i].root);
		db_dup_key_free(db, db->free_list[i].node->key);
#ifdef DB_ENABLE_STATS
		if (stats.db_node_free != (unsigned int)~0) stats.db_node_free++;
#endif /* DB_ENABLE_STATS */
		ers_free(db->nodes, db->free_list[i].node);
	}
	db->free_count = 0;
}

/*****************************************************************************\
 *  (3) Section of protected functions used internally.                      *
 *  NOTE: the protected functions used in the database interface are in the  *
 *           next section.                                                   *
 *  db_int_cmp         - Default comparator for DB_INT databases.            *
 *  db_uint_cmp        - Default comparator for DB_UINT databases.           *
 *  db_string_cmp      - Default comparator for DB_STRING databases.         *
 *  db_istring_cmp     - Default comparator for DB_ISTRING databases.        *
 *  db_int_hash        - Default hasher for DB_INT databases.                *
 *  db_uint_hash       - Default hasher for DB_UINT databases.               *
 *  db_string_hash     - Default hasher for DB_STRING databases.             *
 *  db_istring_hash    - Default hasher for DB_ISTRING databases.            *
 *  db_release_nothing - Releaser that releases nothing.                     *
 *  db_release_key     - Releaser that only releases the key.                *
 *  db_release_data    - Releaser that only releases the data.               *
 *  db_release_both    - Releaser that releases key and data.                *
\*****************************************************************************/

/**
 * Default comparator for DB_INT databases.
 * Compares key1 to key2.
 * Return 0 if equal, negative if lower and positive if higher.
 * <code>maxlen</code> is ignored.
 * @param key1 Key to be compared
 * @param key2 Key being compared to
 * @param maxlen Maximum length of the key to hash
 * @return 0 if equal, negative if lower and positive if higher
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_INT
 * @see common\db.h#DBComparator
 * @see #db_default_cmp(DBType)
 */
static int db_int_cmp(DBKey key1, DBKey key2, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_int_cmp != (unsigned int)~0) stats.db_int_cmp++;
#endif /* DB_ENABLE_STATS */
	if (key1.i < key2.i) return -1;
	if (key1.i > key2.i) return 1;
	return 0;
}

/**
 * Default comparator for DB_UINT databases.
 * Compares key1 to key2.
 * Return 0 if equal, negative if lower and positive if higher.
 * <code>maxlen</code> is ignored.
 * @param key1 Key to be compared
 * @param key2 Key being compared to
 * @param maxlen Maximum length of the key to hash
 * @return 0 if equal, negative if lower and positive if higher
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_UINT
 * @see common\db.h#DBComparator
 * @see #db_default_cmp(DBType)
 */
static int db_uint_cmp(DBKey key1, DBKey key2, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_uint_cmp != (unsigned int)~0) stats.db_uint_cmp++;
#endif /* DB_ENABLE_STATS */
	if (key1.ui < key2.ui) return -1;
	if (key1.ui > key2.ui) return 1;
	return 0;
}

/**
 * Default comparator for DB_STRING databases.
 * Compares key1 to key2.
 * Return 0 if equal, negative if lower and positive if higher.
 * @param key1 Key to be compared
 * @param key2 Key being compared to
 * @param maxlen Maximum length of the key to hash
 * @return 0 if equal, negative if lower and positive if higher
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_STRING
 * @see common\db.h#DBComparator
 * @see #db_default_cmp(DBType)
 */
static int db_string_cmp(DBKey key1, DBKey key2, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_string_cmp != (unsigned int)~0) stats.db_string_cmp++;
#endif /* DB_ENABLE_STATS */
	if (maxlen == 0) maxlen = (unsigned short)~0;
	return strncmp((const char *)key1.str, (const char *)key2.str, maxlen);
}

/**
 * Default comparator for DB_ISTRING databases.
 * Compares key1 to key2 case insensitively.
 * Return 0 if equal, negative if lower and positive if higher.
 * @param key1 Key to be compared
 * @param key2 Key being compared to
 * @param maxlen Maximum length of the key to hash
 * @return 0 if equal, negative if lower and positive if higher
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_ISTRING
 * @see common\db.h#DBComparator
 * @see #db_default_cmp(DBType)
 */
static int db_istring_cmp(DBKey key1, DBKey key2, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_istring_cmp != (unsigned int)~0) stats.db_istring_cmp++;
#endif /* DB_ENABLE_STATS */
	if (maxlen == 0) maxlen = (unsigned short)~0;
	return strncasecmp((const char *)key1.str, (const char *)key2.str, maxlen);
}

/**
 * Default hasher for DB_INT databases.
 * Returns the value of the key as an unsigned int.
 * <code>maxlen</code> is ignored.
 * @param key Key to be hashed
 * @param maxlen Maximum length of the key to hash
 * @return hash of the key
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_INT
 * @see common\db.h#DBHasher
 * @see #db_default_hash(DBType)
 */
static unsigned int db_int_hash(DBKey key, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_int_hash != (unsigned int)~0) stats.db_int_hash++;
#endif /* DB_ENABLE_STATS */
	return (unsigned int)key.i;
}

/**
 * Default hasher for DB_UINT databases.
 * Just returns the value of the key.
 * <code>maxlen</code> is ignored.
 * @param key Key to be hashed
 * @param maxlen Maximum length of the key to hash
 * @return hash of the key
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_UINT
 * @see #db_default_hash(DBType)
 */
static unsigned int db_uint_hash(DBKey key, unsigned short maxlen)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_uint_hash != (unsigned int)~0) stats.db_uint_hash++;
#endif /* DB_ENABLE_STATS */
	return key.ui;
}

/**
 * Default hasher for DB_STRING databases.
 * If maxlen if 0, the maximum number of maxlen is used instead.
 * @param key Key to be hashed
 * @param maxlen Maximum length of the key to hash
 * @return hash of the key
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_STRING
 * @see #db_default_hash(DBType)
 */
static unsigned int db_string_hash(DBKey key, unsigned short maxlen)
{
	unsigned char *k = key.str;
	unsigned int hash = 0;
	unsigned short i;

#ifdef DB_ENABLE_STATS
	if (stats.db_string_hash != (unsigned int)~0) stats.db_string_hash++;
#endif /* DB_ENABLE_STATS */
	if (maxlen == 0)
		maxlen = (unsigned short)~0; // Maximum

	for (i = 0; *k; i++) {
		hash = (hash*33 + *k++)^(hash>>24);
		if (i == maxlen)
			break;
	}

	return hash;
}

/**
 * Default hasher for DB_ISTRING databases.
 * If maxlen if 0, the maximum number of maxlen is used instead.
 * @param key Key to be hashed
 * @param maxlen Maximum length of the key to hash
 * @return hash of the key
 * @see common\db.h#DBKey
 * @see common\db.h\DBType#DB_ISTRING
 * @see #db_default_hash(DBType)
 */
static unsigned int db_istring_hash(DBKey key, unsigned short maxlen)
{
	unsigned char *k = key.str;
	unsigned int hash = 0;
	unsigned short i;

#ifdef DB_ENABLE_STATS
	if (stats.db_istring_hash != (unsigned int)~0) stats.db_istring_hash++;
#endif /* DB_ENABLE_STATS */
	if (maxlen == 0)
		maxlen = (unsigned short)~0; // Maximum

	for (i = 0; *k; i++) {
		hash = (hash*33 + LOWER(*k))^(hash>>24);
		k++;
		if (i == maxlen)
			break;
	}

	return hash;
}

/**
 * Releaser that releases nothing.
 * @param key Key of the database entry
 * @param data Data of the database entry
 * @param which What is being requested to be released
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBRelease
 * @see common\db.h#DBReleaser
 * @see #db_default_releaser(DBType,DBOptions)
 */
static void db_release_nothing(DBKey key, void *data, DBRelease which)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_release_nothing != (unsigned int)~0) stats.db_release_nothing++;
#endif /* DB_ENABLE_STATS */
}

/**
 * Releaser that only releases the key.
 * @param key Key of the database entry
 * @param data Data of the database entry
 * @param which What is being requested to be released
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBRelease
 * @see common\db.h#DBReleaser
 * @see #db_default_release(DBType,DBOptions)
 */
static void db_release_key(DBKey key, void *data, DBRelease which)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_release_key != (unsigned int)~0) stats.db_release_key++;
#endif /* DB_ENABLE_STATS */
	if (which&DB_RELEASE_KEY) aFree(key.str); // needs to be a pointer
}

/**
 * Releaser that only releases the data.
 * @param key Key of the database entry
 * @param data Data of the database entry
 * @param which What is being requested to be released
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBRelease
 * @see common\db.h#DBReleaser
 * @see #db_default_release(DBType,DBOptions)
 */
static void db_release_data(DBKey key, void *data, DBRelease which)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_release_data != (unsigned int)~0) stats.db_release_data++;
#endif /* DB_ENABLE_STATS */
	if (which&DB_RELEASE_DATA) aFree(data);
}

/**
 * Releaser that releases both key and data.
 * @param key Key of the database entry
 * @param data Data of the database entry
 * @param which What is being requested to be released
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBRelease
 * @see common\db.h#DBReleaser
 * @see #db_default_release(DBType,DBOptions)
 */
static void db_release_both(DBKey key, void *data, DBRelease which)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_release_both != (unsigned int)~0) stats.db_release_both++;
#endif /* DB_ENABLE_STATS */
	if (which&DB_RELEASE_KEY) aFree(key.str); // needs to be a pointer
	if (which&DB_RELEASE_DATA) aFree(data);
}

/*****************************************************************************\
 *  (4) Section with protected functions used in the interface of the        *
 *  database.                                                                *
 *  db_obj_get      - Get the data identified by the key.                    *
 *  db_obj_vgetall  - Get the data of the matched entries.                   *
 *  db_obj_getall   - Get the data of the matched entries.                   *
 *  db_obj_vensure  - Get the data identified by the key, creating if it     *
 *           doesn't exist yet.                                              *
 *  db_obj_ensure   - Get the data identified by the key, creating if it     *
 *           doesn't exist yet.                                              *
 *  db_obj_put      - Put data identified by the key in the database.        *
 *  db_obj_remove   - Remove an entry from the database.                     *
 *  db_obj_vforeach - Apply a function to every entry in the database.       *
 *  db_obj_foreach  - Apply a function to every entry in the database.       *
 *  db_obj_vclear   - Remove all entries from the database.                  *
 *  db_obj_clear    - Remove all entries from the database.                  *
 *  db_obj_vdestroy - Destroy the database, freeing all the used memory.     *
 *  db_obj_destroy  - Destroy the database, freeing all the used memory.     *
 *  db_obj_size     - Return the size of the database.                       *
 *  db_obj_type     - Return the type of the database.                       *
 *  db_obj_options  - Return the options of the database.                    *
\*****************************************************************************/

/**
 * Get the data of the entry identifid by the key.
 * @param self Interface of the database
 * @param key Key that identifies the entry
 * @return Data of the entry or NULL if not found
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#get(DBInterface,DBKey)
 */
static void *db_obj_get(DBInterface self, DBKey key)
{
	Database db = (Database)self;
	DBNode node;
	int c;
	void *data = NULL;

#ifdef DB_ENABLE_STATS
	if (stats.db_get != (unsigned int)~0) stats.db_get++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return NULL; // nullpo candidate
	if (!(db->options&DB_OPT_ALLOW_NULL_KEY) && db_is_key_null(db->type, key)) {
		ShowError("db_get: Attempted to retrieve non-allowed NULL key for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}

	db_free_lock(db);
	node = db->ht[db->hash(key, db->maxlen)%HASH_SIZE];
	while (node) {
		c = db->cmp(key, node->key, db->maxlen);
		if (c == 0) {
			data = node->data;
			break;
		}
		if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	db_free_unlock(db);
	return data;
}

/**
 * Get the data of the entries matched by <code>match</code>.
 * It puts a maximum of <code>max</code> entries into <code>buf</code>.
 * If <code>buf</code> is NULL, it only counts the matches.
 * Returns the number of entries that matched.
 * NOTE: if the value returned is greater than <code>max</code>, only the 
 * first <code>max</code> entries found are put into the buffer.
 * @param self Interface of the database
 * @param buf Buffer to put the data of the matched entries
 * @param max Maximum number of data entries to be put into buf
 * @param match Function that matches the database entries
 * @param ... Extra arguments for match
 * @return The number of entries that matched
 * @protected
 * @see common\db.h#DBInterface
 * @see common\db.h#DBMatcher(DBKey key, void *data, va_list args)
 * @see common\db.h\DBInterface#vgetall(DBInterface,void **,unsigned int,DBMatch,va_list)
 */
static unsigned int db_obj_vgetall(DBInterface self, void **buf, unsigned int max, DBMatcher match, va_list args)
{
	Database db = (Database)self;
	unsigned int i;
	DBNode node;
	DBNode parent;
	unsigned int ret = 0;

#ifdef DB_ENABLE_STATS
	if (stats.db_vgetall != (unsigned int)~0) stats.db_vgetall++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return 0; // nullpo candidate
	if (match == NULL) return 0; // nullpo candidate

	db_free_lock(db);
	for (i = 0; i < HASH_SIZE; i++) {
		// Match in the order: current node, left tree, right tree
		node = db->ht[i];
		while (node) {
			parent = node->parent;
			if (!(node->deleted) && match(node->key, node->data, args) == 0) {
				if (buf && ret < max)
					buf[ret] = node->data;
				ret++;
			}
			if (node->left) {
				node = node->left;
				continue;
			}
			if (node->right) {
				node = node->right;
				continue;
			}
			while (node) {
				parent = node->parent;
				if (parent && parent->right && parent->left == node) {
					node = parent->right;
					break;
				}
				node = parent;
			}
		}
	}
	db_free_unlock(db);
	return ret;
}

/**
 * Just calls {@link common\db.h\DBInterface#vgetall(DBInterface,void **,unsigned int,DBMatch,va_list)}.
 * Get the data of the entries matched by <code>match</code>.
 * It puts a maximum of <code>max</code> entries into <code>buf</code>.
 * If <code>buf</code> is NULL, it only counts the matches.
 * Returns the number of entries that matched.
 * NOTE: if the value returned is greater than <code>max</code>, only the 
 * first <code>max</code> entries found are put into the buffer.
 * @param self Interface of the database
 * @param buf Buffer to put the data of the matched entries
 * @param max Maximum number of data entries to be put into buf
 * @param match Function that matches the database entries
 * @param ... Extra arguments for match
 * @return The number of entries that matched
 * @protected
 * @see common\db.h#DBMatcher(DBKey key, void *data, va_list args)
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vgetall(DBInterface,void **,unsigned int,DBMatch,va_list)
 * @see common\db.h\DBInterface#getall(DBInterface,void **,unsigned int,DBMatch,...)
 */
static unsigned int db_obj_getall(DBInterface self, void **buf, unsigned int max, DBMatcher match, ...)
{
	va_list args;
	unsigned int ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_getall != (unsigned int)~0) stats.db_getall++;
#endif /* DB_ENABLE_STATS */
	if (self == NULL) return 0; // nullpo candidate

	va_start(args, match);
	ret = self->vgetall(self, buf, max, match, args);
	va_end(args);
	return ret;
}

/**
 * Get the data of the entry identified by the key.
 * If the entry does not exist, an entry is added with the data returned by 
 * <code>create</code>.
 * @param self Interface of the database
 * @param key Key that identifies the entry
 * @param create Function used to create the data if the entry doesn't exist
 * @param args Extra arguments for create
 * @return Data of the entry
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBCreateData
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vensure(DBInterface,DBKey,DBCreateData,va_list)
 */
static void *db_obj_vensure(DBInterface self, DBKey key, DBCreateData create, va_list args)
{
	Database db = (Database)self;
	DBNode node;
	DBNode parent = NULL;
	unsigned int hash;
	int c = 0;
	void *data = NULL;

#ifdef DB_ENABLE_STATS
	if (stats.db_vensure != (unsigned int)~0) stats.db_vensure++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return NULL; // nullpo candidate
	if (create == NULL) {
		ShowError("db_ensure: Create function is NULL for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}
	if (!(db->options&DB_OPT_ALLOW_NULL_KEY) && db_is_key_null(db->type, key)) {
		ShowError("db_ensure: Attempted to use non-allowed NULL key for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}

	db_free_lock(db);
	hash = db->hash(key, db->maxlen)%HASH_SIZE;
	node = db->ht[hash];
	while (node) {
		c = db->cmp(key, node->key, db->maxlen);
		if (c == 0) {
			break;
		}
		parent = node;
		if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	// Create node if necessary
	if (node == NULL) {
		if (db->item_count == (unsigned int)~0) {
			ShowError("db_vensure: item_count overflow, aborting item insertion.\n"
					"Database allocated at %s:%d",
					db->alloc_file, db->alloc_line);
				return NULL;
		}
#ifdef DB_ENABLE_STATS
		if (stats.db_node_alloc != (unsigned int)~0) stats.db_node_alloc++;
#endif /* DB_ENABLE_STATS */
		node = ers_alloc(db->nodes, struct dbn);
		node->left = NULL;
		node->right = NULL;
		node->deleted = 0;
		db->item_count++;
		if (c == 0) { // hash entry is empty
			node->color = BLACK;
			node->parent = NULL;
			db->ht[hash] = node;
		} else {
			node->color = RED;
			if (c < 0) { // put at the left
				parent->left = node;
				node->parent = parent;
			} else { // put at the right
				parent->right = node;
				node->parent = parent;
			}
			if (parent->color == RED) // two consecutive RED nodes, must rebalance
				db_rebalance(node, &db->ht[hash]);
		}
		// put key and data in the node
		if (db->options&DB_OPT_DUP_KEY) {
			node->key = db_dup_key(db, key);
			if (db->options&DB_OPT_RELEASE_KEY)
				db->release(key, data, DB_RELEASE_KEY);
		} else {
			node->key = key;
		}
		node->data = create(key, args);
	}
	data = node->data;
	db_free_unlock(db);
	return data;
}

/**
 * Just calls {@link common\db.h\DBInterface#vensure(DBInterface,DBKey,DBCreateData,va_list)}.
 * Get the data of the entry identified by the key.
 * If the entry does not exist, an entry is added with the data returned by 
 * <code>create</code>.
 * @param self Interface of the database
 * @param key Key that identifies the entry
 * @param create Function used to create the data if the entry doesn't exist
 * @param ... Extra arguments for create
 * @return Data of the entry
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBCreateData
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vensure(DBInterface,DBKey,DBCreateData,va_list)
 * @see common\db.h\DBInterface#ensure(DBInterface,DBKey,DBCreateData,...)
 */
static void *db_obj_ensure(DBInterface self, DBKey key, DBCreateData create, ...)
{
	va_list args;
	void *ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_ensure != (unsigned int)~0) stats.db_ensure++;
#endif /* DB_ENABLE_STATS */
	if (self == NULL) return 0; // nullpo candidate

	va_start(args, create);
	ret = self->vensure(self, key, create, args);
	va_end(args);
	return ret;
}

/**
 * Put the data identified by the key in the database.
 * Returns the previous data if the entry exists or NULL.
 * NOTE: Uses the new key, the old one is released.
 * @param self Interface of the database
 * @param key Key that identifies the data
 * @param data Data to be put in the database
 * @return The previous data if the entry exists or NULL
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBInterface
 * @see #db_malloc_dbn(void)
 * @see common\db.h\DBInterface#put(DBInterface,DBKey,void *)
 */
static void *db_obj_put(DBInterface self, DBKey key, void *data)
{
	Database db = (Database)self;
	DBNode node;
	DBNode parent = NULL;
	int c = 0;
	unsigned int hash;
	void *old_data = NULL;

#ifdef DB_ENABLE_STATS
	if (stats.db_put != (unsigned int)~0) stats.db_put++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return NULL; // nullpo candidate
	if (db->global_lock) {
		ShowError("db_put: Database is being destroyed, aborting entry insertion.\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}
	if (!(db->options&DB_OPT_ALLOW_NULL_KEY) && db_is_key_null(db->type, key)) {
		ShowError("db_put: Attempted to use non-allowed NULL key for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
	  	return NULL; // nullpo candidate
	}
	if (!(data || db->options&DB_OPT_ALLOW_NULL_DATA)) {
		ShowError("db_put: Attempted to use non-allowed NULL data for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}

	if (db->item_count == (unsigned int)~0) {
		ShowError("db_put: item_count overflow, aborting item insertion.\n"
				"Database allocated at %s:%d",
				db->alloc_file, db->alloc_line);
		return NULL;
	}
	// search for an equal node
	db_free_lock(db);
	hash = db->hash(key, db->maxlen)%HASH_SIZE;
	for (node = db->ht[hash]; node; ) {
		c = db->cmp(key, node->key, db->maxlen);
		if (c == 0) { // equal entry, replace
			if (node->deleted) {
				db_free_remove(db, node);
			} else {
				db->release(node->key, node->data, DB_RELEASE_BOTH);
			}
			old_data = node->data;
			break;
		}
		parent = node;
		if (c < 0) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
	// allocate a new node if necessary
	if (node == NULL) {
#ifdef DB_ENABLE_STATS
		if (stats.db_node_alloc != (unsigned int)~0) stats.db_node_alloc++;
#endif /* DB_ENABLE_STATS */
		node = ers_alloc(db->nodes, struct dbn);
		node->left = NULL;
		node->right = NULL;
		node->deleted = 0;
		db->item_count++;
		if (c == 0) { // hash entry is empty
			node->color = BLACK;
			node->parent = NULL;
			db->ht[hash] = node;
		} else {
			node->color = RED;
			if (c < 0) { // put at the left
				parent->left = node;
				node->parent = parent;
			} else { // put at the right
				parent->right = node;
				node->parent = parent;
			}
			if (parent->color == RED) // two consecutive RED nodes, must rebalance
				db_rebalance(node, &db->ht[hash]);
		}
	}
	// put key and data in the node
	if (db->options&DB_OPT_DUP_KEY) {
		node->key = db_dup_key(db, key);
		if (db->options&DB_OPT_RELEASE_KEY)
			db->release(key, data, DB_RELEASE_KEY);
	} else {
		node->key = key;
	}
	node->data = data;
	db_free_unlock(db);
	return old_data;
}

/**
 * Remove an entry from the database.
 * Returns the data of the entry.
 * NOTE: The key (of the database) is released in {@link #db_free_add(Database,DBNode,DBNode *)}.
 * @param self Interface of the database
 * @param key Key that identifies the entry
 * @return The data of the entry or NULL if not found
 * @protected
 * @see common\db.h#DBKey
 * @see common\db.h#DBInterface
 * @see #db_free_add(Database,DBNode,DBNode *)
 * @see common\db.h\DBInterface#remove(DBInterface,DBKey)
 */
static void *db_obj_remove(DBInterface self, DBKey key)
{
	Database db = (Database)self;
	void *data = NULL;
	DBNode node;
	unsigned int hash;
	int c = 0;

#ifdef DB_ENABLE_STATS
	if (stats.db_remove != (unsigned int)~0) stats.db_remove++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return NULL; // nullpo candidate
	if (db->global_lock) {
		ShowError("db_remove: Database is being destroyed. Aborting entry deletion.\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}
	if (!(db->options&DB_OPT_ALLOW_NULL_KEY) && db_is_key_null(db->type, key))	{
		ShowError("db_remove: Attempted to use non-allowed NULL key for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return NULL; // nullpo candidate
	}

	db_free_lock(db);
	hash = db->hash(key, db->maxlen)%HASH_SIZE;
	for(node = db->ht[hash]; node; ){
		c = db->cmp(key, node->key, db->maxlen);
		if (c == 0) {
			if (!(node->deleted)) {
				data = node->data;
				db->release(node->key, node->data, DB_RELEASE_DATA);
				db_free_add(db, node, &db->ht[hash]);
			}
			break;
		}
		if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	db_free_unlock(db);
	return data;
}

/**
 * Apply <code>func</code> to every entry in the database.
 * Returns the sum of values returned by func.
 * @param self Interface of the database
 * @param func Function to be applyed
 * @param args Extra arguments for func
 * @return Sum of the values returned by func
 * @protected
 * @see common\db.h#DBInterface
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h\DBInterface#vforeach(DBInterface,DBApply,va_list)
 */
static int db_obj_vforeach(DBInterface self, DBApply func, va_list args)
{
	Database db = (Database)self;
	unsigned int i;
	int sum = 0;
	DBNode node;
	DBNode parent;

#ifdef DB_ENABLE_STATS
	if (stats.db_vforeach != (unsigned int)~0) stats.db_vforeach++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return 0; // nullpo candidate
	if (func == NULL) {
		ShowError("db_foreach: Passed function is NULL for db allocated at %s:%d\n",db->alloc_file, db->alloc_line);
		return 0; // nullpo candidate
	}

	db_free_lock(db);
	for (i = 0; i < HASH_SIZE; i++) {
		// Apply func in the order: current node, left node, right node
		node = db->ht[i];
		while (node) {
			parent = node->parent;
			if (!(node->deleted))
				sum += func(node->key, node->data, args);
			if (node->left) {
				node = node->left;
				continue;
			}
			if (node->right) {
				node = node->right;
				continue;
			}
			while (node) {
				parent = node->parent;
				if (parent && parent->right && parent->left == node) {
					node = parent->right;
					break;
				}
				node = parent;
			}
		}
	}
	db_free_unlock(db);
	return sum;
}

/**
 * Just calls {@link common\db.h\DBInterface#vforeach(DBInterface,DBApply,va_list)}.
 * Apply <code>func</code> to every entry in the database.
 * Returns the sum of values returned by func.
 * @param self Interface of the database
 * @param func Function to be applyed
 * @param ... Extra arguments for func
 * @return Sum of the values returned by func
 * @protected
 * @see common\db.h#DBInterface
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h\DBInterface#vforeach(DBInterface,DBApply,va_list)
 * @see common\db.h\DBInterface#foreach(DBInterface,DBApply,...)
 */
static int db_obj_foreach(DBInterface self, DBApply func, ...)
{
	va_list args;
	int ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_foreach != (unsigned int)~0) stats.db_foreach++;
#endif /* DB_ENABLE_STATS */
	if (self == NULL) return 0; // nullpo candidate

	va_start(args, func);
	ret = self->vforeach(self, func, args);
	va_end(args);
	return ret;
}

/**
 * Removes all entries from the database.
 * Before deleting an entry, func is applyed to it.
 * Releases the key and the data.
 * Returns the sum of values returned by func, if it exists.
 * @param self Interface of the database
 * @param func Function to be applyed to every entry before deleting
 * @param args Extra arguments for func
 * @return Sum of values returned by func
 * @protected
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vclear(DBInterface,DBApply,va_list)
 */
static int db_obj_vclear(DBInterface self, DBApply func, va_list args)
{
	Database db = (Database)self;
	int sum = 0;
	unsigned int i;
	DBNode node;
	DBNode parent;

#ifdef DB_ENABLE_STATS
	if (stats.db_vclear != (unsigned int)~0) stats.db_vclear++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return 0; // nullpo candidate

	db_free_lock(db);
	for (i = 0; i < HASH_SIZE; i++) {
		// Apply the func and delete in the order: left tree, right tree, current node
		node = db->ht[i];
		db->ht[i] = NULL;
		while (node) {
			parent = node->parent;
			if (node->left) {
				node = node->left;
				continue;
			}
			if (node->right) {
				node = node->right;
				continue;
			}
			if (node->deleted) {
				db_dup_key_free(db, node->key);
			} else {
				if (func)
					sum += func(node->key, node->data, args);
				db->release(node->key, node->data, DB_RELEASE_BOTH);
				node->deleted = 1;
			}
#ifdef DB_ENABLE_STATS
			if (stats.db_node_free != (unsigned int)~0) stats.db_node_free++;
#endif /* DB_ENABLE_STATS */
			ers_free(db->nodes, node);
			if (parent) {
				if (parent->left == node)
					parent->left = NULL;
				else
					parent->right = NULL;
			}
			node = parent;
		}
		db->ht[i] = NULL;
	}
	db->free_count = 0;
	db->item_count = 0;
	db_free_unlock(db);
	return sum;
}

/**
 * Just calls {@link common\db.h\DBInterface#vclear(DBInterface,DBApply,va_list)}.
 * Removes all entries from the database.
 * Before deleting an entry, func is applyed to it.
 * Releases the key and the data.
 * Returns the sum of values returned by func, if it exists.
 * NOTE: This locks the database globally. Any attempt to insert or remove 
 * a database entry will give an error and be aborted (except for clearing).
 * @param self Interface of the database
 * @param func Function to be applyed to every entry before deleting
 * @param ... Extra arguments for func
 * @return Sum of values returned by func
 * @protected
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vclear(DBInterface,DBApply,va_list)
 * @see common\db.h\DBInterface#clear(DBInterface,DBApply,...)
 */
static int db_obj_clear(DBInterface self, DBApply func, ...)
{
	va_list args;
	int ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_clear != (unsigned int)~0) stats.db_clear++;
#endif /* DB_ENABLE_STATS */
	if (self == NULL) return 0; // nullpo candidate

	va_start(args, func);
	ret = self->vclear(self, func, args);
	va_end(args);
	return ret;
}

/**
 * Finalize the database, feeing all the memory it uses.
 * Before deleting an entry, func is applyed to it.
 * Returns the sum of values returned by func, if it exists.
 * NOTE: This locks the database globally. Any attempt to insert or remove 
 * a database entry will give an error and be aborted (except for clearing).
 * @param self Interface of the database
 * @param func Function to be applyed to every entry before deleting
 * @param args Extra arguments for func
 * @return Sum of values returned by func
 * @protected
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vdestroy(DBInterface,DBApply,va_list)
 */
static int db_obj_vdestroy(DBInterface self, DBApply func, va_list args)
{
	Database db = (Database)self;
	int sum;

#ifdef DB_ENABLE_STATS
	if (stats.db_vdestroy != (unsigned int)~0) stats.db_vdestroy++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return 0; // nullpo candidate
	if (db->global_lock) {
		ShowError("db_vdestroy: Database is already locked for destruction. Aborting second database destruction.\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line);
		return 0;
	}
	if (db->free_lock)
		ShowWarning("db_vdestroy: Database is still in use, %u lock(s) left. Continuing database destruction.\n"
				"Database allocated at %s:%d\n",
				db->alloc_file, db->alloc_line, db->free_lock);

#ifdef DB_ENABLE_STATS
	switch (db->type) {
		case DB_INT:
			stats.db_int_destroy++;
			break;
		case DB_UINT:
			stats.db_uint_destroy++;
			break;
		case DB_STRING:
			stats.db_string_destroy++;
			break;
		case DB_ISTRING:
			stats.db_istring_destroy++;
			break;
	}
#endif /* DB_ENABLE_STATS */
	db_free_lock(db);
	db->global_lock = 1;
	sum = self->vclear(self, func, args);
	aFree(db->free_list);
	db->free_list = NULL;
	db->free_max = 0;
	ers_destroy(db->nodes);
	db_free_unlock(db);
	aFree(db);
	return sum;
}

/**
 * Just calls {@link common\db.h\DBInterface#db_vdestroy(DBInterface,DBApply,va_list)}.
 * Finalize the database, feeing all the memory it uses.
 * Before deleting an entry, func is applyed to it.
 * Releases the key and the data.
 * Returns the sum of values returned by func, if it exists.
 * NOTE: This locks the database globally. Any attempt to insert or remove 
 * a database entry will give an error and be aborted.
 * @param self Interface of the database
 * @param func Function to be applyed to every entry before deleting
 * @param ... Extra arguments for func
 * @return Sum of values returned by func
 * @protected
 * @see common\db.h#DBApply(DBKey,void *,va_list)
 * @see common\db.h#DBInterface
 * @see common\db.h\DBInterface#vdestroy(DBInterface,DBApply,va_list)
 * @see common\db.h\DBInterface#destroy(DBInterface,DBApply,...)
 */
static int db_obj_destroy(DBInterface self, DBApply func, ...)
{
	va_list args;
	int ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_destroy != (unsigned int)~0) stats.db_destroy++;
#endif /* DB_ENABLE_STATS */
	if (self == NULL) return 0; // nullpo candidate

	va_start(args, func);
	ret = self->vdestroy(self, func, args);
	va_end(args);
	return ret;
}

/**
 * Return the size of the database (number of items in the database).
 * @param self Interface of the database
 * @return Size of the database
 * @protected
 * @see common\db.h#DBInterface
 * @see Database#item_count
 * @see common\db.h\DBInterface#size(DBInterface)
 */
static unsigned int db_obj_size(DBInterface self)
{
	Database db = (Database)self;
	unsigned int item_count;

#ifdef DB_ENABLE_STATS
	if (stats.db_size != (unsigned int)~0) stats.db_size++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return 0; // nullpo candidate

	db_free_lock(db);
	item_count = db->item_count;
	db_free_unlock(db);

	return item_count;
}

/**
 * Return the type of database.
 * @param self Interface of the database
 * @return Type of the database
 * @protected
 * @see common\db.h#DBType
 * @see common\db.h#DBInterface
 * @see Database#type
 * @see common\db.h\DBInterface#type(DBInterface)
 */
static DBType db_obj_type(DBInterface self)
{
	Database db = (Database)self;
	DBType type;

#ifdef DB_ENABLE_STATS
	if (stats.db_type != (unsigned int)~0) stats.db_type++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return -1; // nullpo candidate - TODO what should this return?

	db_free_lock(db);
	type = db->type;
	db_free_unlock(db);

	return type;
}

/**
 * Return the options of the database.
 * @param self Interface of the database
 * @return Options of the database
 * @protected
 * @see common\db.h#DBOptions
 * @see common\db.h#DBInterface
 * @see Database#options
 * @see common\db.h\DBInterface#options(DBInterface)
 */
static DBOptions db_obj_options(DBInterface self)
{
	Database db = (Database)self;
	DBOptions options;

#ifdef DB_ENABLE_STATS
	if (stats.db_options != (unsigned int)~0) stats.db_options++;
#endif /* DB_ENABLE_STATS */
	if (db == NULL) return DB_OPT_BASE; // nullpo candidate - TODO what should this return?

	db_free_lock(db);
	options = db->options;
	db_free_unlock(db);

	return options;
}

/*****************************************************************************\
 *  (5) Section with public functions.                                       *
 *  db_fix_options     - Apply database type restrictions to the options.    *
 *  db_default_cmp     - Get the default comparator for a type of database.  *
 *  db_default_hash    - Get the default hasher for a type of database.      *
 *  db_default_release - Get the default releaser for a type of database     *
 *           with the specified options.                                     *
 *  db_custom_release  - Get a releaser that behaves a certains way.         *
 *  db_alloc           - Allocate a new database.                            *
 *  db_i2key           - Manual cast from 'int' to 'DBKey'.                  *
 *  db_ui2key          - Manual cast from 'unsigned int' to 'DBKey'.         *
 *  db_str2key         - Manual cast from 'unsigned char *' to 'DBKey'.      *
 *  db_init            - Initialize the database system.                     *
 *  db_final           - Finalize the database system.                       *
\*****************************************************************************/

/**
 * Returns the fixed options according to the database type.
 * Sets required options and unsets unsupported options.
 * For numeric databases DB_OPT_DUP_KEY and DB_OPT_RELEASE_KEY are unset.
 * @param type Type of the database
 * @param options Original options of the database
 * @return Fixed options of the database
 * @private
 * @see common\db.h#DBType
 * @see common\db.h#DBOptions
 * @see #db_default_release(DBType,DBOptions)
 * @see #db_alloc(const char *,int,DBType,DBOptions,unsigned short)
 * @see common\db.h#db_fix_options(DBType,DBOptions)
 */
DBOptions db_fix_options(DBType type, DBOptions options)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_fix_options != (unsigned int)~0) stats.db_fix_options++;
#endif /* DB_ENABLE_STATS */
	switch (type) {
		case DB_INT:
		case DB_UINT: // Numeric database, do nothing with the keys
			return options&~(DB_OPT_DUP_KEY|DB_OPT_RELEASE_KEY);

		default:
			ShowError("db_fix_options: Unknown database type %u with options %x\n", type, options);
		case DB_STRING:
		case DB_ISTRING: // String databases, no fix required
			return options;
	}
}

/**
 * Returns the default comparator for the specified type of database.
 * @param type Type of database
 * @return Comparator for the type of database or NULL if unknown database
 * @public
 * @see common\db.h#DBType
 * @see #db_int_cmp(DBKey,DBKey,unsigned short)
 * @see #db_uint_cmp(DBKey,DBKey,unsigned short)
 * @see #db_string_cmp(DBKey,DBKey,unsigned short)
 * @see #db_istring_cmp(DBKey,DBKey,unsigned short)
 * @see common\db.h#db_default_cmp(DBType)
 */
DBComparator db_default_cmp(DBType type)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_default_cmp != (unsigned int)~0) stats.db_default_cmp++;
#endif /* DB_ENABLE_STATS */
	switch (type) {
		case DB_INT:     return db_int_cmp;
		case DB_UINT:    return db_uint_cmp;
		case DB_STRING:  return db_string_cmp;
		case DB_ISTRING: return db_istring_cmp;
		default:
			ShowError("db_default_cmp: Unknown database type %u\n", type);
			return NULL;
	}
}

/**
 * Returns the default hasher for the specified type of database.
 * @param type Type of database
 * @return Hasher of the type of database or NULL if unknown database
 * @public
 * @see common\db.h#DBType
 * @see #db_int_hash(DBKey,unsigned short)
 * @see #db_uint_hash(DBKey,unsigned short)
 * @see #db_string_hash(DBKey,unsigned short)
 * @see #db_istring_hash(DBKey,unsigned short)
 * @see common\db.h#db_default_hash(DBType)
 */
DBHasher db_default_hash(DBType type)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_default_hash != (unsigned int)~0) stats.db_default_hash++;
#endif /* DB_ENABLE_STATS */
	switch (type) {
		case DB_INT:     return db_int_hash;
		case DB_UINT:    return db_uint_hash;
		case DB_STRING:  return db_string_hash;
		case DB_ISTRING: return db_istring_hash;
		default:
			ShowError("db_default_hash: Unknown database type %u\n", type);
			return NULL;
	}
}

/**
 * Returns the default releaser for the specified type of database with the 
 * specified options.
 * NOTE: the options are fixed with {@link #db_fix_options(DBType,DBOptions)}
 * before choosing the releaser.
 * @param type Type of database
 * @param options Options of the database
 * @return Default releaser for the type of database with the specified options
 * @public
 * @see common\db.h#DBType
 * @see common\db.h#DBOptions
 * @see common\db.h#DBReleaser
 * @see #db_release_nothing(DBKey,void *,DBRelease)
 * @see #db_release_key(DBKey,void *,DBRelease)
 * @see #db_release_data(DBKey,void *,DBRelease)
 * @see #db_release_both(DBKey,void *,DBRelease)
 * @see #db_custom_release(DBRelease)
 * @see common\db.h#db_default_release(DBType,DBOptions)
 */
DBReleaser db_default_release(DBType type, DBOptions options)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_default_release != (unsigned int)~0) stats.db_default_release++;
#endif /* DB_ENABLE_STATS */
	options = db_fix_options(type, options);
	if (options&DB_OPT_RELEASE_DATA) { // Release data, what about the key?
		if (options&(DB_OPT_DUP_KEY|DB_OPT_RELEASE_KEY))
			return db_release_both; // Release both key and data
		return db_release_data; // Only release data
	}
	if (options&(DB_OPT_DUP_KEY|DB_OPT_RELEASE_KEY))
		return db_release_key; // Only release key
	return db_release_nothing; // Release nothing
}

/**
 * Returns the releaser that releases the specified release options.
 * @param which Options that specified what the releaser releases
 * @return Releaser for the specified release options
 * @public
 * @see common\db.h#DBRelease
 * @see common\db.h#DBReleaser
 * @see #db_release_nothing(DBKey,void *,DBRelease)
 * @see #db_release_key(DBKey,void *,DBRelease)
 * @see #db_release_data(DBKey,void *,DBRelease)
 * @see #db_release_both(DBKey,void *,DBRelease)
 * @see #db_default_release(DBType,DBOptions)
 * @see common\db.h#db_custom_release(DBRelease)
 */
DBReleaser db_custom_release(DBRelease which)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_custom_release != (unsigned int)~0) stats.db_custom_release++;
#endif /* DB_ENABLE_STATS */
	switch (which) {
		case DB_RELEASE_NOTHING: return db_release_nothing;
		case DB_RELEASE_KEY:     return db_release_key;
		case DB_RELEASE_DATA:    return db_release_data;
		case DB_RELEASE_BOTH:    return db_release_both;
		default:
			ShowError("db_custom_release: Unknown release options %u\n", which);
			return NULL;
	}
}

/**
 * Allocate a new database of the specified type.
 * NOTE: the options are fixed by {@link #db_fix_options(DBType,DBOptions)}
 * before creating the database.
 * @param file File where the database is being allocated
 * @param line Line of the file where the database is being allocated
 * @param type Type of database
 * @param options Options of the database
 * @param maxlen Maximum length of the string to be used as key in string 
 *          databases
 * @return The interface of the database
 * @public
 * @see common\db.h#DBType
 * @see common\db.h#DBInterface
 * @see #Database
 * @see #db_fix_options(DBType,DBOptions)
 * @see common\db.h#db_alloc(const char *,int,DBType,unsigned short)
 */
DBInterface db_alloc(const char *file, int line, DBType type, DBOptions options, unsigned short maxlen)
{
	Database db;
	unsigned int i;

#ifdef DB_ENABLE_STATS
	if (stats.db_alloc != (unsigned int)~0) stats.db_alloc++;
	switch (type) {
		case DB_INT:
			stats.db_int_alloc++;
			break;
		case DB_UINT:
			stats.db_uint_alloc++;
			break;
		case DB_STRING:
			stats.db_string_alloc++;
			break;
		case DB_ISTRING:
			stats.db_istring_alloc++;
			break;
	}
#endif /* DB_ENABLE_STATS */
	CREATE(db, struct db, 1);

	options = db_fix_options(type, options);
	/* Interface of the database */
	db->dbi.get      = db_obj_get;
	db->dbi.getall   = db_obj_getall;
	db->dbi.vgetall  = db_obj_vgetall;
	db->dbi.ensure   = db_obj_ensure;
	db->dbi.vensure  = db_obj_vensure;
	db->dbi.put      = db_obj_put;
	db->dbi.remove   = db_obj_remove;
	db->dbi.foreach  = db_obj_foreach;
	db->dbi.vforeach = db_obj_vforeach;
	db->dbi.clear    = db_obj_clear;
	db->dbi.vclear   = db_obj_vclear;
	db->dbi.destroy  = db_obj_destroy;
	db->dbi.vdestroy = db_obj_vdestroy;
	db->dbi.size     = db_obj_size;
	db->dbi.type     = db_obj_type;
	db->dbi.options  = db_obj_options;
	/* File and line of allocation */
	db->alloc_file = file;
	db->alloc_line = line;
	/* Lock system */
	db->free_list = NULL;
	db->free_count = 0;
	db->free_max = 0;
	db->free_lock = 0;
	/* Other */
	db->nodes = ers_new((uint32)sizeof(struct dbn));
	db->cmp = db_default_cmp(type);
	db->hash = db_default_hash(type);
	db->release = db_default_release(type, options);
	for (i = 0; i < HASH_SIZE; i++)
		db->ht[i] = NULL;
	db->type = type;
	db->options = options;
	db->item_count = 0;
	db->maxlen = maxlen;
	db->global_lock = 0;

	return &db->dbi;
}

#ifdef DB_MANUAL_CAST_TO_UNION
/**
 * Manual cast from 'int' to the union DBKey.
 * Created for compilers that don't support casting to unions.
 * @param key Key to be casted
 * @return The key as a DBKey union
 * @public
 * @see common\db.h#DB_MANUAL_CAST_TO_UNION
 * @see #db_ui2key(unsigned int)
 * @see #db_str2key(unsigned char *)
 * @see common\db.h#db_i2key(int)
 */
DBKey db_i2key(int key)
{
	DBKey ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_i2key != (unsigned int)~0) stats.db_i2key++;
#endif /* DB_ENABLE_STATS */
	ret.i = key;
	return ret;
}

/**
 * Manual cast from 'unsigned int' to the union DBKey.
 * Created for compilers that don't support casting to unions.
 * @param key Key to be casted
 * @return The key as a DBKey union
 * @public
 * @see common\db.h#DB_MANUAL_CAST_TO_UNION
 * @see #db_i2key(int)
 * @see #db_str2key(unsigned char *)
 * @see common\db.h#db_ui2key(unsigned int)
 */
DBKey db_ui2key(unsigned int key)
{
	DBKey ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_ui2key != (unsigned int)~0) stats.db_ui2key++;
#endif /* DB_ENABLE_STATS */
	ret.ui = key;
	return ret;
}

/**
 * Manual cast from 'unsigned char *' to the union DBKey.
 * Created for compilers that don't support casting to unions.
 * @param key Key to be casted
 * @return The key as a DBKey union
 * @public
 * @see common\db.h#DB_MANUAL_CAST_TO_UNION
 * @see #db_i2key(int)
 * @see #db_ui2key(unsigned int)
 * @see common\db.h#db_str2key(unsigned char *)
 */
DBKey db_str2key(unsigned char *key)
{
	DBKey ret;

#ifdef DB_ENABLE_STATS
	if (stats.db_str2key != (unsigned int)~0) stats.db_str2key++;
#endif /* DB_ENABLE_STATS */
	ret.str = key;
	return ret;
}
#endif /* DB_MANUAL_CAST_TO_UNION */

/**
 * Initialize the database system.
 * @public
 * @see #db_final(void)
 * @see common\db.h#db_init(void)
 */
void db_init(void)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_init != (unsigned int)~0) stats.db_init++;
#endif /* DB_ENABLE_STATS */
}

/**
 * Finalize the database system.
 * Frees the memory used by the block reusage system.
 * @public
 * @see common\db.h#DB_FINAL_NODE_CHECK
 * @see #db_init(void)
 * @see common\db.h#db_final(void)
 */
void db_final(void)
{
#ifdef DB_ENABLE_STATS
	if (stats.db_final != (unsigned int)~0)
		stats.db_final++;
	ShowInfo(CL_WHITE"Database nodes"CL_RESET":\n"
			"allocated %u, freed %u\n",
			stats.db_node_alloc, stats.db_node_free);
	ShowInfo(CL_WHITE"Database types"CL_RESET":\n"
			"DB_INT     : allocated %10u, destroyed %10u\n"
			"DB_UINT    : allocated %10u, destroyed %10u\n"
			"DB_STRING  : allocated %10u, destroyed %10u\n"
			"DB_ISTRING : allocated %10u, destroyed %10u\n",
			stats.db_int_alloc,     stats.db_int_destroy,
			stats.db_uint_alloc,    stats.db_uint_destroy,
			stats.db_string_alloc,  stats.db_string_destroy,
			stats.db_istring_alloc, stats.db_istring_destroy);
	ShowInfo(CL_WHITE"Database function counters"CL_RESET":\n"
			"db_rotate_left     %10u, db_rotate_right    %10u,\n"
			"db_rebalance       %10u, db_rebalance_erase %10u,\n"
			"db_is_key_null     %10u,\n"
			"db_dup_key         %10u, db_dup_key_free    %10u,\n"
			"db_free_add        %10u, db_free_remove     %10u,\n"
			"db_free_lock       %10u, db_free_unlock     %10u,\n"
			"db_int_cmp         %10u, db_uint_cmp        %10u,\n"
			"db_string_cmp      %10u, db_istring_cmp     %10u,\n"
			"db_int_hash        %10u, db_uint_hash       %10u,\n"
			"db_string_hash     %10u, db_istring_hash    %10u,\n"
			"db_release_nothing %10u, db_release_key     %10u,\n"
			"db_release_data    %10u, db_release_both    %10u,\n"
			"db_get             %10u,\n"
			"db_getall          %10u, db_vgetall         %10u,\n"
			"db_ensure          %10u, db_vensure         %10u,\n"
			"db_put             %10u, db_remove          %10u,\n"
			"db_foreach         %10u, db_vforeach        %10u,\n"
			"db_clear           %10u, db_vclear          %10u,\n"
			"db_destroy         %10u, db_vdestroy        %10u,\n"
			"db_size            %10u, db_type            %10u,\n"
			"db_options         %10u, db_fix_options     %10u,\n"
			"db_default_cmp     %10u, db_default_hash    %10u,\n"
			"db_default_release %10u, db_custom_release  %10u,\n"
			"db_alloc           %10u, db_i2key           %10u,\n"
			"db_ui2key          %10u, db_str2key         %10u,\n"
			"db_init            %10u, db_final           %10u\n",
			stats.db_rotate_left,     stats.db_rotate_right,
			stats.db_rebalance,       stats.db_rebalance_erase,
			stats.db_is_key_null,
			stats.db_dup_key,         stats.db_dup_key_free,
			stats.db_free_add,        stats.db_free_remove,
			stats.db_free_lock,       stats.db_free_unlock,
			stats.db_int_cmp,         stats.db_uint_cmp,
			stats.db_string_cmp,      stats.db_istring_cmp,
			stats.db_int_hash,        stats.db_uint_hash,
			stats.db_string_hash,     stats.db_istring_hash,
			stats.db_release_nothing, stats.db_release_key,
			stats.db_release_data,    stats.db_release_both,
			stats.db_get,
			stats.db_getall,          stats.db_vgetall,
			stats.db_ensure,          stats.db_vensure,
			stats.db_put,             stats.db_remove,
			stats.db_foreach,         stats.db_vforeach,
			stats.db_clear,           stats.db_vclear,
			stats.db_destroy,         stats.db_vdestroy,
			stats.db_size,            stats.db_type,
			stats.db_options,         stats.db_fix_options,
			stats.db_default_cmp,     stats.db_default_hash,
			stats.db_default_release, stats.db_custom_release,
			stats.db_alloc,           stats.db_i2key,
			stats.db_ui2key,          stats.db_str2key,
			stats.db_init,            stats.db_final);
#endif /* DB_ENABLE_STATS */
}

