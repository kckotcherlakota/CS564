//
// File: wl.h
// 
//  Description: This file contains the data structure for word locator. 
//  Student Name: Karan Talreja
//  UW Campus ID: 9075678186
//  enamil: talreja2@wisc.edu
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>

class listNode;
class list;

///@brief Type to specify the type of input command.
typedef enum _command {
  LOAD_e,     ///< "load" command
  LOCATE_e,   ///< "locate" command
  NEW_e,      ///< "new" command
  END_e,      ///< "end" command
  ERROR_e     ///< Default error type
} command_e;

std::string delim;
char output[1000] = {0};

///@brief Helper macro to print to stdout using write system call.
#define wl_printf(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDOUT_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDOUT\n");

/**
 * @brief Function to create a deliminator string to be used for tokenizing input file via strtok_r.
 * @return Nothing
 */
void createDelim() {
  for(int i = 1; i < 256; i++) {
    if (i >= 48 && i <= 57) continue;   //[0-9] cannot end word
    if (i >= 65 && i <= 90) continue;   //[A-Z] cannot end word
    if (i >= 97 && i <= 122) continue;  //[a-z] cannot end word
    if (i == 39) continue;              //'(Apostrophe) cannot end word
    delim += (char)i;
  }
}

/**
 * @brief Function to check whether the input command argument contains correct characters.
 * @retval true   If no character is out of the range of valid characters
 * @retval false  If any character is out of the range of valid characters
 */
bool checkCorrectChars(const char* word) {
  for (; *word != '\0'; word++) {
    int i = *word;
    if (i >= 48 && i <= 57) continue;         //[0-9] valid 
    else if (i >= 65 && i <= 90) continue;    //[A-Z] valid 
    else if (i >= 97 && i <= 122) continue;   //[a-z] valid 
    else if (i == 39) continue;               //'(Apostrophe) cannot end word
    else if (i == 0) continue;
    else return false;
  }
  return true;
}

/**
 * @brief Class to represent the node of a binary search tree.
 */
class listNode {
  public:
    listNode* next;   ///< Member pointer to hold the address of next node in the list.
    int index;        ///< Data to be stored in the list's node

    ///@brief Default constructor which sets all member pointers to NULL
    listNode() : next(NULL), index(-1) {}
    
    /**
     * @brief One argument constructor with input argument to set index.
     * @param i  Input word's index to be stored in this node.
     */
    listNode(int i) : next(NULL), index(i) {}

};

/**
 * @brief Class to represent the node of a binary search tree.
 */
class list {
  private:
    size_t m_size;
    listNode* head;
    listNode* curr;

  public:
    
    ///@brief Default constructor which sets all member pointers to NULL
    list(): m_size(0),head(NULL), curr(NULL) {}
    
    /**
     * @brief One argument constructor with input argument to set word.
     * @param word  Input word to be stored in this node.
     */
    list(listNode* h) : m_size(1),head(h), curr(h) {}

    void push_back(int idx) {
      if (head == NULL) {
        head = new listNode(idx);
        curr = head;
        m_size++;
      } else {
        curr->next = new listNode(idx);
        curr = curr->next;
        m_size++;
      }
    }

    size_t size() {
      return m_size;
    }

    int operator [] (int index) {
      if (index == 0) return head->index;
      listNode* retVal = head;
      for (int i =0; i < index; i++) {
        retVal = retVal->next;
      }
      return retVal->index;
    }
};

/**
 * @brief Class to represent the node of a binary search tree.
 */
class node {
  public:
    node* left;               ///< Member Pointer to left subtree of this node.
    node* right;              ///< Member Pointer to right subtree of this node.
    char* word;               ///< Member for Word stored in this node.
    //std::vector<int> indexs;   ///< Indices of this word in the input document
    list* index;

    ///@brief Default constructor which sets all member pointers to NULL
    node():left(NULL), right(NULL), word(NULL), index(NULL) {}

    /**
     * @brief One argument constructor with input argument to set word.
     * @param word  Input word to be stored in this node.
     */
    node(char* word):left(NULL), right(NULL), word(word), index(new list()) {}
    
    ///@brief Destructor for memory deallocation.
    ~node() {
      if (NULL != word) free(word);
      if (NULL != left) delete left;
      if (NULL != right) delete right;
    }
} *root;

/**
 * @brief Function to insert a word into binary search tree.
 * @param	word	word from the document which is to be inserted into the tree.
 * @param index Index at which this word was found in the document.
 * @return The new root of the binary search tree.
 */
node* insert(node* root, const char* word, int index) {
  if (root == NULL) {
    root = new node(strdup(word));
    root->index->push_back(index);
    return root;
  }
  int rc = strcasecmp(word, root->word);
  if (rc < 0) {
    node* retNode = insert(root->left, word, index);
    if (root->left == NULL) root->left = retNode;
  } else if (rc > 0) {
    node* retNode = insert(root->right, word, index);
    if (root->right == NULL) root->right = retNode;
  } else root->index->push_back(index);
  return root;
}

/**
 * @brief Function to lookup a word from the binary search tree.
 * @param	root	The root of the binary search tree in which the node is to be looked up.
 * @param word  The word which is to be looked for
 * @details The function returns if the input is NULL. If not it checks if the input word
 *          is equal to, less than or greater than the current node's word. If it's equal
 *          current node is returned. Otherwise the word is lookedup into left subtree or
 *          right sub tree.
 * @return The node which contains the information for the query word. If it doesn't exist
 *         NULL is returned.
 */
node* lookup(node* root, const char* word) {
  if (root == NULL) return NULL;
  node* retNode = NULL;
  int rc = strcasecmp(word, root->word);
  if (rc < 0) retNode = lookup(root->left, word);
  else if (rc > 0) retNode = lookup(root->right, word);
  else retNode = root;
  return retNode;
}

/**
 * @brief Function to do a inorder traversal on the binary search tree.
 * @param	root	The root of the binary search tree on which in order traversal is to be done.
 * @return Nothing.
 */
//void inorder(node* root) {
//  if (root == NULL) return;
//  inorder(root->left);
//  wl_printf("%s ",root->word);
//  for (std::vector<int>::iterator itr = root->index.begin(); itr != root->index.end(); itr++) {
//    wl_printf("%d ",*itr);
//  }
//  wl_printf("\n");
//  inorder(root->right);
//}
