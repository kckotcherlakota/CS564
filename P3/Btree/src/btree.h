/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#pragma once

#include <iostream>
#include <string>
#include "string.h"
#include <sstream>
#include <vector>

#include "types.h"
#include "page.h"
#include "file.h"
#include "buffer.h"
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/end_of_file_exception.h"
#include <assert.h>

//#define DEBUG

#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

namespace badgerdb
{

/**
 * @brief Datatype enumeration type.
 */
enum Datatype
{
	INTEGER = 0,
	DOUBLE = 1,
	STRING = 2
};

/**
 * @brief Scan operations enumeration. Passed to BTreeIndex::startScan() method.
 */
enum Operator
{ 
	LT, 	/* Less Than */
	LTE,	/* Less Than or Equal to */
	GTE,	/* Greater Than or Equal to */
	GT		/* Greater Than */
};

/**
 * @brief Size of String key.
 */
const  int STRINGSIZE = 10;

/**
 * @brief Number of key slots in B+Tree leaf for INTEGER key.
 */
//                                                  sibling ptr             key               rid
const  int INTARRAYLEAFSIZE = ( Page::SIZE - sizeof( PageId ) ) / ( sizeof( int ) + sizeof( RecordId ) );

/**
 * @brief Number of key slots in B+Tree leaf for DOUBLE key.
 */
//                                                     sibling ptr               key               rid
const  int DOUBLEARRAYLEAFSIZE = ( Page::SIZE - sizeof( PageId ) ) / ( sizeof( double ) + sizeof( RecordId ) );

/**
 * @brief Number of key slots in B+Tree leaf for STRING key.
 */
//                                                    sibling ptr           key                      rid
const  int STRINGARRAYLEAFSIZE = ( Page::SIZE - sizeof( PageId ) ) / ( 10 * sizeof(char) + sizeof( RecordId ) );

/**
 * @brief Number of key slots in B+Tree non-leaf for INTEGER key.
 */
//                                                     level     extra pageNo                  key       pageNo
const  int INTARRAYNONLEAFSIZE = ( Page::SIZE - sizeof( int ) - sizeof( PageId ) ) / ( sizeof( int ) + sizeof( PageId ) );

/**
 * @brief Number of key slots in B+Tree leaf for DOUBLE key.
 */
//                                                        level        extra pageNo                 key            pageNo   -1 due to structure padding
const  int DOUBLEARRAYNONLEAFSIZE = (( Page::SIZE - sizeof( int ) - sizeof( PageId ) ) / ( sizeof( double ) + sizeof( PageId ) )) - 1;

/**
 * @brief Number of key slots in B+Tree leaf for STRING key.
 */
//                                                        level        extra pageNo             key                   pageNo
const  int STRINGARRAYNONLEAFSIZE = ( Page::SIZE - sizeof( int ) - sizeof( PageId ) ) / ( 10 * sizeof(char) + sizeof( PageId ) );

/**
 * @brief Structure to store a key-rid pair. It is used to pass the pair to functions that 
 * add to or make changes to the leaf node pages of the tree. Is templated for the key member.
 */
template <class T>
class RIDKeyPair{
public:
	RecordId rid;
	T key;
	void set( RecordId r, T k)
	{
		rid = r;
		key = k;
	}
};

/**
 * @brief Structure to store a key page pair which is used to pass the key and page to functions that make 
 * any modifications to the non leaf pages of the tree.
*/
template <class T>
class PageKeyPair{
public:
	PageId pageNo;
	T key;
	void set( int p, T k)
	{
		pageNo = p;
		key = k;
	}
};

/**
 * @brief Overloaded operator to compare the key values of two rid-key pairs
 * and if they are the same compares to see if the first pair has
 * a smaller rid.pageNo value.
*/
template <class T>
bool operator<( const RIDKeyPair<T>& r1, const RIDKeyPair<T>& r2 )
{
	if( r1.key != r2.key )
		return r1.key < r2.key;
	else
		return r1.rid.page_number < r2.rid.page_number;
}

/**
 * @brief The meta page, which holds metadata for Index file, is always first page of the btree index file and is cast
 * to the following structure to store or retrieve information from it.
 * Contains the relation name for which the index is created, the byte offset
 * of the key value on which the index is made, the type of the key and the page no
 * of the root page. Root page starts as page 2 but since a split can occur
 * at the root the root page may get moved up and get a new page no.
*/
struct IndexMetaInfo{
  /**
   * Name of base relation.
   */
	char relationName[20];

  /**
   * Offset of attribute, over which index is built, inside the record stored in pages.
   */
	int attrByteOffset;

  /**
   * Type of the attribute over which index is built.
   */
	Datatype attrType;

  /**
   * Page number of root page of the B+ Tree inside the file index file.
   */
	PageId rootPageNo;
};

/*
Each node is a page, so once we read the page in we just cast the pointer to the page to this struct and use it to access the parts
These structures basically are the format in which the information is stored in the pages for the index file depending on what kind of 
node they are. The level memeber of each non leaf structure seen below is set to 1 if the nodes 
at this level are just above the leaf nodes. Otherwise set to 0.
*/

/**
 * @brief Structure for all non-leaf nodes when the key is of INTEGER type.
*/
struct NonLeafNodeInt{
  /**
   * Level of the node in the tree.
   */
	int level;

  /**
   * Stores keys.
   */
	int keyArray[ INTARRAYNONLEAFSIZE ];

  /**
   * Stores page numbers of child pages which themselves are other non-leaf/leaf nodes in the tree.
   */
	PageId pageNoArray[ INTARRAYNONLEAFSIZE + 1 ];
};

/**
 * @brief Structure for all non-leaf nodes when the key is of DOUBLE type.
*/
struct NonLeafNodeDouble{
  /**
   * Level of the node in the tree.
   */
	int level;

  /**
   * Stores keys.
   */
	double keyArray[ DOUBLEARRAYNONLEAFSIZE ];

  /**
   * Stores page numbers of child pages which themselves are other non-leaf/leaf nodes in the tree.
   */
	PageId pageNoArray[ DOUBLEARRAYNONLEAFSIZE + 1 ];
};

/**
 * @brief Structure for all non-leaf nodes when the key is of STRING type.
*/
struct NonLeafNodeString{
  /**
   * Level of the node in the tree.
   */
	int level;

  /**
   * Stores keys.
   */
	char keyArray[ STRINGARRAYNONLEAFSIZE ][ STRINGSIZE ];

  /**
   * Stores page numbers of child pages which themselves are other non-leaf/leaf nodes in the tree.
   */
	PageId pageNoArray[ STRINGARRAYNONLEAFSIZE + 1 ];
};

/**
 * @brief Structure for all leaf nodes when the key is of INTEGER type.
*/
struct LeafNodeInt{
  /**
   * Stores keys.
   */
	int keyArray[ INTARRAYLEAFSIZE ];

  /**
   * Stores RecordIds.
   */
	RecordId ridArray[ INTARRAYLEAFSIZE ];

  /**
   * Page number of the leaf on the right side.
	 * This linking of leaves allows to easily move from one leaf to the next leaf during index scan.
   */
	PageId rightSibPageNo;
};

/**
 * @brief Structure for all leaf nodes when the key is of DOUBLE type.
*/
struct LeafNodeDouble{
  /**
   * Stores keys.
   */
	double keyArray[ DOUBLEARRAYLEAFSIZE ];

  /**
   * Stores RecordIds.
   */
	RecordId ridArray[ DOUBLEARRAYLEAFSIZE ];

  /**
   * Page number of the leaf on the right side.
	 * This linking of leaves allows to easily move from one leaf to the next leaf during index scan.
   */
	PageId rightSibPageNo;
};

/**
 * @brief Structure for all leaf nodes when the key is of STRING type.
*/
struct LeafNodeString{
  /**
   * Stores keys.
   */
	char keyArray[ STRINGARRAYLEAFSIZE ][ STRINGSIZE ];

  /**
   * Stores RecordIds.
   */
	RecordId ridArray[ STRINGARRAYLEAFSIZE ];

  /**
   * Page number of the leaf on the right side.
	 * This linking of leaves allows to easily move from one leaf to the next leaf during index scan.
   */
	PageId rightSibPageNo;
};

/**
 * @brief BTreeIndex class. It implements a B+ Tree index on a single attribute of a
 * relation. This index supports only one scan at a time.
*/
class BTreeIndex {

 private:

  /**
   * File object for the index file.
   */
	File		*file;

  /**
   * Buffer Manager Instance.
   */
	BufMgr	*bufMgr;

  /**
   * Page number of meta page.
   */
	PageId	headerPageNum;

  /**
   * page number of root page of B+ tree inside index file.
   */
	PageId	rootPageNum;

  /**
   * Datatype of attribute over which index is built.
   */
	Datatype	attributeType;

  /**
   * Offset of attribute, over which index is built, inside records. 
   */
	int 		attrByteOffset;

  /**
   * Number of keys in leaf node, depending upon the type of key.
   */
	int		leafOccupancy;

  /**
   * Number of keys in non-leaf node, depending upon the type of key.
   */
	int		nodeOccupancy;


	// MEMBERS SPECIFIC TO SCANNING

  /**
   * True if an index scan has been started.
   */
	bool		scanExecuting;

  /**
   * Index of next entry to be scanned in current leaf being scanned.
   */
	int		nextEntry;

  /**
   * Page number of current page being scanned.
   */
	PageId	currentPageNum;

  /**
   * Current Page being scanned.
   */
	Page		*currentPageData;

  /**
   * Low INTEGER value for scan.
   */
	int		lowValInt;

  /**
   * Low DOUBLE value for scan.
   */
	double	lowValDouble;

  /**
   * Low STRING value for scan.
   */
	std::string	lowValString;

  /**
   * High INTEGER value for scan.
   */
	int		highValInt;

  /**
   * High DOUBLE value for scan.
   */
	double	highValDouble;

  /**
   * High STRING value for scan.
   */
	std::string highValString;
	
  /**
   * Low Operator. Can only be GT(>) or GTE(>=).
   */
	Operator	lowOp;

  /**
   * High Operator. Can only be LT(<) or LTE(<=).
   */
	Operator	highOp;

	
 public:

  /**
   * BTreeIndex Constructor. 
	 * Check to see if the corresponding index file exists. If so, open the file.
	 * If not, create it and insert entries for every tuple in the base relation using FileScan class.
   *
   * @param relationName        Name of file.
   * @param outIndexName        Return the name of index file.
   * @param bufMgrIn						Buffer Manager Instance
   * @param attrByteOffset			Offset of attribute, over which index is to be built, in the record
   * @param attrType						Datatype of attribute over which index is built
   * @throws  BadIndexInfoException     If the index file already exists for the corresponding attribute, but values in metapage(relationName, attribute byte offset, attribute type etc.) do not match with values received through constructor parameters.
   */
	BTreeIndex(const std::string & relationName, std::string & outIndexName,
						BufMgr *bufMgrIn,	const int attrByteOffset,	const Datatype attrType);
	

  /**
   * BTreeIndex Destructor. 
	 * End any initialized scan, flush index file, after unpinning any pinned pages, from the buffer manager
	 * and delete file instance thereby closing the index file.
	 * Destructor should not throw any exceptions. All exceptions should be caught in here itself. 
	 * */
	~BTreeIndex();


  /**
	 * Insert a new entry using the pair <value,rid>. 
	 * Start from root to recursively find out the leaf to insert the entry in. The insertion may cause splitting of leaf node.
	 * This splitting will require addition of new leaf page number entry into the parent non-leaf, which may in-turn get split.
	 * This may continue all the way upto the root causing the root to get split. If root gets split, metapage needs to be changed accordingly.
	 * Make sure to unpin pages as soon as you can.
   * @param key			Key to insert, pointer to integer/double/char string
   * @param rid			Record ID of a record whose entry is getting inserted into the index.
	**/
	const void insertEntry(const void* key, const RecordId rid);


  /**
	 * Begin a filtered scan of the index.  For instance, if the method is called 
	 * using ("a",GT,"d",LTE) then we should seek all entries with a value 
	 * greater than "a" and less than or equal to "d".
	 * If another scan is already executing, that needs to be ended here.
	 * Set up all the variables for scan. Start from root to find out the leaf page that contains the first RecordID
	 * that satisfies the scan parameters. Keep that page pinned in the buffer pool.
   * @param lowVal	Low value of range, pointer to integer / double / char string
   * @param lowOp		Low operator (GT/GTE)
   * @param highVal	High value of range, pointer to integer / double / char string
   * @param highOp	High operator (LT/LTE)
   * @throws  BadOpcodesException If lowOp and highOp do not contain one of their their expected values 
   * @throws  BadScanrangeException If lowVal > highval
	 * @throws  NoSuchKeyFoundException If there is no key in the B+ tree that satisfies the scan criteria.
	**/
	const void startScan(const void* lowVal, const Operator lowOp, const void* highVal, const Operator highOp);


  /**
	 * Fetch the record id of the next index entry that matches the scan.
	 * Return the next record from current page being scanned. If current page has been scanned to its entirety, move on to the right sibling of current page, if any exists, to start scanning that page. Make sure to unpin any pages that are no longer required.
   * @param outRid	RecordId of next record found that satisfies the scan criteria returned in this
	 * @throws ScanNotInitializedException If no scan has been initialized.
	 * @throws IndexScanCompletedException If no more records, satisfying the scan criteria, are left to be scanned.
	**/
	const void scanNext(RecordId& outRid);  // returned record id


  /**
	 * Terminate the current scan. Unpin any pinned pages. Reset scan specific variables.
	 * @throws ScanNotInitializedException If no scan has been initialized.
	**/
	const void endScan();
private:
  template <typename keyType> class keyTraits;

	template <typename keyType, typename traits>
	const void startScanTemplate(const void* lowVal, const void* highVal);

  template <typename keyType, typename traits>
  const void scanNextTemplate(RecordId& outRid);

	template <typename keyType, typename traits>
  void getPageNoAndOffsetOfKeyInsert(const void* key, Page* rootPage, PageId& pageNo, int& insertAt, int& endOfRecordsOffset, PageId& lastPageNo, bool insert = true);

	template <typename keyType, typename traits>
	void createRoot(Page* rootPage);

  template <typename keyType, typename traits>
  const void insertKeyTemplate(const void* key, const RecordId rid);
};

// This is the structure for tuples in the base relation

typedef struct tuple {
	int i;
	double d;
	char s[64];
} RECORD;

template<>
class BTreeIndex::keyTraits<int> {
public:
   typedef LeafNodeInt leafType;
   typedef NonLeafNodeInt nonLeafType;
   static const int LEAFSIZE = INTARRAYLEAFSIZE;
   static const int NONLEAFSIZE = INTARRAYNONLEAFSIZE;
   static void setScanBounds(BTreeIndex* index, const void* lowValParm, const void* highValParm) {
     index->lowValInt = *reinterpret_cast<int*>(const_cast<void*>(lowValParm));
     index->highValInt = *reinterpret_cast<int*>(const_cast<void*>(highValParm));
   }

   static inline int getLowBound(BTreeIndex* index) {
     return index->lowValInt;
   }

   static inline int getUpperBound(BTreeIndex* index) {
     return index->highValInt;
   }
};

template<>
class BTreeIndex::keyTraits<double> {
public:
   typedef LeafNodeDouble leafType;
   typedef NonLeafNodeDouble nonLeafType;
   static const int LEAFSIZE = DOUBLEARRAYLEAFSIZE;
   static const int NONLEAFSIZE = DOUBLEARRAYNONLEAFSIZE;
   static void setScanBounds(BTreeIndex* index, const void* lowValParm, const void* highValParm) {
     index->lowValDouble = *reinterpret_cast<double*>(const_cast<void*>(lowValParm));
     index->highValDouble = *reinterpret_cast<double*>(const_cast<void*>(highValParm));
   }

   static inline double getLowBound(BTreeIndex* index) {
     return index->lowValDouble;
   }

   static inline double getUpperBound(BTreeIndex* index) {
     return index->highValDouble;
   }
};

template<typename keyType, typename traits=BTreeIndex::keyTraits<keyType> >
void BTreeIndex::createRoot(Page* rootPage) {
  typedef typename traits::nonLeafType nonLeafType;
  nonLeafType rootData;
  memset(&rootData, 0, sizeof(nonLeafType));
  rootData.level = 1;
  memcpy(rootPage, &rootData, sizeof(nonLeafType));
  this->bufMgr->unPinPage(this->file, this->rootPageNum, true);
}

template<typename keyType, typename traits=BTreeIndex::keyTraits<keyType> >
void BTreeIndex::getPageNoAndOffsetOfKeyInsert(const void* key, Page* rootPage, PageId& pageNo, int& insertAt, int& endOfRecordsOffset, PageId& lastPageNo, bool insert)
{
  typedef typename traits::leafType leafType;
  typedef typename traits::nonLeafType nonLeafType;
  int i = 0, depth = 1;
  keyType keyValue = *reinterpret_cast<keyType*>(const_cast<void*>(key));
  nonLeafType* rootData = reinterpret_cast<nonLeafType*>(rootPage);
  nonLeafType* currPage = rootData;
  // <going to pade index , coming from page id>
  std::vector<std::pair<int,PageId>> pathOfTraversal;
  PageId lastPage = this->rootPageNum;
  Page* tempPage;
  while (depth < rootData->level) {
    if (keyValue < currPage->keyArray[0]) {
      // Case smaller than all keys
      i = 0;
      pathOfTraversal.push_back(std::pair<int,PageId>(i, lastPage));
    } else {
      // invariant page[i] contains keys >= key[i-1]
      // invariant page[i] contains keys < key [i]
      for (i = 0; i < traits::NONLEAFSIZE; i++) {
        if (currPage->pageNoArray[i+1] == Page::INVALID_NUMBER) {
          pathOfTraversal.push_back(std::pair<int,PageId>(i, lastPage));
          break;
        }
        /* 1st page contains keys greater than key[0] so if keyValue is greater than key[1]:
         * the key must lie in page[2] or ahead. Since page[2] contains keys greater than key[1] */
        if (currPage->keyArray[i] <= keyValue) {
          // If the next page is not invalid, it might contain the key, so continue.
#ifdef DEBUG
          // keys all smaller than keyArray[i] should lie in this page so it should be valid
          assert(currPage->pageNoArray[i] != Page::INVALID_NUMBER);
          // keys all greater than equal to keyArray[i] should lie in page[i+1] or ahead.
          assert(currPage->pageNoArray[i+1] != Page::INVALID_NUMBER);
#endif
            continue; // means if this was the last page in the node, we need to add to this page only otherwise continue
        }
        pathOfTraversal.push_back(std::pair<int,PageId>(i, lastPage));
        break;
      }
    }
    if (i == traits::NONLEAFSIZE) {
      pathOfTraversal.push_back(std::pair<int,PageId>(i, lastPage));
    }
    // TODO karantalreja : if i == traits::NONLEAFSIZE then need to split page
    this->bufMgr->unPinPage(this->file, lastPage, false);
    this->bufMgr->readPage(this->file, currPage->pageNoArray[i], tempPage);
    lastPage = currPage->pageNoArray[i];
    currPage = reinterpret_cast<nonLeafType*>(tempPage);
    depth++;
  }
  pageNo = lastPage;
  i = 0;
  insertAt = traits::LEAFSIZE;
  leafType* dataPage = reinterpret_cast<leafType*>(currPage);
  for (i = 0; i < traits::LEAFSIZE; i++) {
    if (dataPage->ridArray[i].page_number == Page::INVALID_NUMBER) {
      if (insertAt == traits::LEAFSIZE) insertAt = i;
      break;
    }
    if (keyValue > dataPage->keyArray[i]) continue;
    if (insertAt == traits::LEAFSIZE) {
      insertAt = i;
      if (insert == false) break;
    }
  }
  bool done = false;
  if (i == traits::LEAFSIZE) {
    // split data page
    Page* greaterKey;
    int medianIdx = traits::LEAFSIZE/2;
    PageId GparentPageId;
    int Goffset;
    nonLeafType* GparentData;
    int k=0;
    while (pathOfTraversal.size() >= 1) {
      PageId parentPageId;
      int offset;
      Page* parentPage;
      nonLeafType* parentData;
      parentPageId = pathOfTraversal.back().second;
      offset = pathOfTraversal.back().first;  // The page idx in parent pageArray in which the key wants to go.
      pathOfTraversal.pop_back();
      this->bufMgr->readPage(this->file, parentPageId, parentPage);
      parentData = reinterpret_cast<nonLeafType*>(parentPage);
      if (done == false) {
        GparentPageId = parentPageId;
        Goffset = offset;
        GparentData = parentData;
      }
      for (k = offset; k <= traits::NONLEAFSIZE; k++) {
        if (parentData->pageNoArray[k] == Page::INVALID_NUMBER) break;
      }

      // Split parent page
      if (k == traits::NONLEAFSIZE+1) {
        Page* greaterParentPage;
        int medianIdxParent = traits::NONLEAFSIZE/2;
        nonLeafType newRootData;
        Page* newRoot;
        int parentParentOffset = 0;
        PageId parentParentPageId;
        if (pathOfTraversal.empty()) {
          this->bufMgr->allocPage(this->file, this->rootPageNum, newRoot);
          parentParentPageId = this->rootPageNum;
          memset(&newRootData, 0, sizeof(nonLeafType));
          newRootData.level = parentData->level+1;
          newRootData.pageNoArray[0] = parentPageId;
        } else {
          parentParentPageId = pathOfTraversal.back().second;
          this->bufMgr->readPage(this->file, parentParentPageId, newRoot);
          newRootData = *reinterpret_cast<nonLeafType*>(newRoot);
          parentParentOffset = pathOfTraversal.back().first;
        }
        for (k = parentParentOffset; k <= traits::NONLEAFSIZE; k++) {
          if (newRootData.pageNoArray[k] == Page::INVALID_NUMBER) break;
        }
        for (; k > parentParentOffset; k--) {
          if (k-1 >= 0) newRootData.pageNoArray[k] = newRootData.pageNoArray[k-1];
          if (k-2 >= 0) newRootData.keyArray[k-1] = newRootData.keyArray[k-2];
        }
#ifdef DEBUG
        assert(newRootData.pageNoArray[parentParentOffset+1] == Page::INVALID_NUMBER || newRootData.pageNoArray[parentParentOffset] == newRootData.pageNoArray[parentParentOffset+1]);
#endif
        this->bufMgr->allocPage(this->file, newRootData.pageNoArray[parentParentOffset+1], greaterParentPage);
        newRootData.keyArray[parentParentOffset] = parentData->keyArray[medianIdxParent];

        nonLeafType dataPageRight;
        memset(&dataPageRight, 0, sizeof(nonLeafType));
        dataPageRight.level = parentData->level;
        for (int i = medianIdxParent+1, j = 0; i < traits::NONLEAFSIZE; i++,j++) {
          dataPageRight.keyArray[j] = parentData->keyArray[i];
          dataPageRight.pageNoArray[j+1] = parentData->pageNoArray[i+1];
          parentData->keyArray[i] = 0;
          parentData->pageNoArray[i+1] = Page::INVALID_NUMBER;
        }

        dataPageRight.pageNoArray[0] = parentData->pageNoArray[medianIdxParent+1];
        parentData->pageNoArray[medianIdxParent+1] = Page::INVALID_NUMBER;
        parentData->keyArray[medianIdxParent] = 0;

        if (done == false) {
          if (keyValue >= newRootData.keyArray[parentParentOffset]) {
            GparentData = reinterpret_cast<nonLeafType*>(greaterParentPage);
            Goffset = offset - medianIdxParent - 1;
            i = Goffset - 1;
            GparentPageId = newRootData.pageNoArray[parentParentOffset+1];
            done = true;
          } else {
            GparentData = parentData;
            Goffset = offset;
            GparentPageId = parentPageId;
            done = true;
          }
        }

        memcpy(newRoot, &newRootData, sizeof(nonLeafType));
        memcpy(greaterParentPage, &dataPageRight, sizeof(nonLeafType));

        this->bufMgr->unPinPage(this->file, parentParentPageId, true);
        if (keyValue >= newRootData.keyArray[parentParentOffset]) {
          this->bufMgr->unPinPage(this->file, newRootData.pageNoArray[parentParentOffset], true);
          if (newRootData.level >= 4)
            this->bufMgr->unPinPage(this->file, newRootData.pageNoArray[parentParentOffset+1], true);
        } else {
          this->bufMgr->unPinPage(this->file, newRootData.pageNoArray[parentParentOffset+1], true);
          if (newRootData.level >= 4)
            this->bufMgr->unPinPage(this->file, newRootData.pageNoArray[parentParentOffset], true);
        }
      } else {
        if (GparentPageId != parentPageId)
          this->bufMgr->unPinPage(this->file, parentPageId, true);
        break;
      }
    }
    PageId parentPageId = GparentPageId;
    int offset = Goffset;
    nonLeafType* parentData = GparentData;

    for (k = offset; k <= traits::NONLEAFSIZE; k++) {
      if (parentData->pageNoArray[k] == Page::INVALID_NUMBER) break;
    }
#ifdef DEBUG
    assert(k != traits::NONLEAFSIZE+1);
#endif
    for (; k > offset; k--) {
      if (k-1 >= 0) parentData->pageNoArray[k] = parentData->pageNoArray[k-1];
      if (k-2 >= 0) parentData->keyArray[k-1] = parentData->keyArray[k-2];
    }
    parentData->keyArray[offset] = dataPage->keyArray[medianIdx];
#ifdef DEBUG
    assert(offset == 0 || parentData->keyArray[offset-1] < parentData->keyArray[offset]);
    if (offset+2 < traits::NONLEAFSIZE && parentData->pageNoArray[offset+2] != Page::INVALID_NUMBER)
      assert(parentData->keyArray[offset+1] > parentData->keyArray[offset]);

    // As insert mode, the above loop should have copied this value to the next slot or
    // if its the first empty slot its value should be Page::INVALID_NUMBER
    assert(parentData->pageNoArray[offset+1] == Page::INVALID_NUMBER || parentData->pageNoArray[offset] == parentData->pageNoArray[offset+1]);
#endif
    this->bufMgr->allocPage(this->file, parentData->pageNoArray[offset+1], greaterKey);
    leafType dataPageRight;
    memset(&dataPageRight, 0, sizeof(leafType));
    dataPageRight.rightSibPageNo = dataPage->rightSibPageNo;
    dataPage->rightSibPageNo = parentData->pageNoArray[offset+1];
#ifdef DEBUG
    assert(insertAt == 0 || dataPage->keyArray[insertAt-1] < keyValue);
    assert(insertAt == traits::LEAFSIZE || dataPage->keyArray[insertAt] > keyValue);
#endif
    if (keyValue > dataPage->keyArray[medianIdx]) {
      insertAt -= medianIdx;
      lastPageNo = pageNo;
      pageNo = parentData->pageNoArray[offset+1];
      endOfRecordsOffset = (traits::LEAFSIZE%2) ? medianIdx+1 : medianIdx;
    } else {
      lastPageNo = pageNo;
      pageNo = lastPage;
      endOfRecordsOffset = medianIdx;
    }
    for (int i = medianIdx, j = 0; i < traits::LEAFSIZE; i++,j++) {
      dataPageRight.keyArray[j] = dataPage->keyArray[i];
      dataPageRight.ridArray[j] = dataPage->ridArray[i];
      dataPage->keyArray[i] = 0;
      dataPage->ridArray[i].page_number = Page::INVALID_NUMBER;
      dataPage->ridArray[i].slot_number = 0;
    }
    memcpy(greaterKey, &dataPageRight, sizeof(leafType));
#ifdef DEBUG
    if (keyValue > dataPageRight.keyArray[0]) {
      assert(insertAt == 0 || dataPageRight.keyArray[insertAt-1] < keyValue);
      assert(insertAt == traits::LEAFSIZE || insertAt == endOfRecordsOffset ||dataPageRight.keyArray[insertAt] > keyValue);
    } else {
      assert(insertAt == 0 || dataPage->keyArray[insertAt-1] < keyValue);
      assert(insertAt == traits::LEAFSIZE || insertAt == endOfRecordsOffset ||dataPage->keyArray[insertAt] > keyValue);
    }
#endif
    this->bufMgr->unPinPage(this->file, lastPage, true);
    this->bufMgr->unPinPage(this->file, parentPageId, true);
    this->bufMgr->unPinPage(this->file, parentData->pageNoArray[offset+1], true);
  } else {
    this->bufMgr->unPinPage(this->file, lastPage, false);
    endOfRecordsOffset = i;
    lastPageNo = pageNo;
  }
  #ifdef DEBUG
  assert(insertAt >= 0);
  assert(insertAt <= endOfRecordsOffset);
  assert(endOfRecordsOffset <= traits::LEAFSIZE);
  #endif
  return;
}


template <typename keyType, class traits=BTreeIndex::keyTraits<keyType> >
const void BTreeIndex::scanNextTemplate(RecordId& outRid) {
  if (this->currentPageData == NULL) throw IndexScanCompletedException();
  typedef typename traits::leafType leafType;
  leafType* dataPage = reinterpret_cast<leafType*>(this->currentPageData);
  if (this->highOp == LT && dataPage->keyArray[this->nextEntry] >= traits::getUpperBound(this)) {
    this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
    throw IndexScanCompletedException();
  }
  if (this->highOp == LTE && dataPage->keyArray[this->nextEntry] > traits::getUpperBound(this)) {
    this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
    throw IndexScanCompletedException();
  }
  outRid = dataPage->ridArray[this->nextEntry];
  #ifdef DEBUG
  assert(outRid.page_number != Page::INVALID_NUMBER);
  assert(outRid.slot_number != 0);
  #endif
  if (this->nextEntry + 1 == traits::LEAFSIZE || dataPage->ridArray[this->nextEntry+1].page_number == Page::INVALID_NUMBER) {
    this->nextEntry = 0;
    this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
    this->currentPageNum = dataPage->rightSibPageNo;
    if (this->currentPageNum == Page::INVALID_NUMBER) {
      this->currentPageData = NULL;
    } else this->bufMgr->readPage(this->file, this->currentPageNum, this->currentPageData);
  } else this->nextEntry++;
}

template <typename keyType, class traits=BTreeIndex::keyTraits<keyType> >
const void BTreeIndex::startScanTemplate(const void* lowVal, const void* highVal) {
  traits::setScanBounds(this, lowVal, highVal);
  typedef typename traits::leafType leafType;
  Page* rootPage;
  this->bufMgr->readPage(this->file, this->rootPageNum, rootPage);
  int insertAt, endOfRecordsOffset;
  PageId dataPageNum, dataPageNumPrev;
  this->getPageNoAndOffsetOfKeyInsert<keyType>(lowVal, rootPage, dataPageNum, insertAt, endOfRecordsOffset, dataPageNumPrev, false);
  if (dataPageNumPrev == dataPageNum) { //TODO karantalreja : Handle the non equal case
    this->currentPageNum = dataPageNum;
    this->bufMgr->readPage(this->file, this->currentPageNum, this->currentPageData);
    this->nextEntry = insertAt;
    leafType* dataPage = reinterpret_cast<leafType*>(this->currentPageData);
    if (dataPage->ridArray[this->nextEntry].page_number == Page::INVALID_NUMBER) {
      if (Page::INVALID_NUMBER != dataPage->rightSibPageNo) {
        this->nextEntry = 0;
        this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
        this->currentPageNum = dataPage->rightSibPageNo;
        this->bufMgr->readPage(this->file, this->currentPageNum, this->currentPageData);
      } else {
        this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
        throw NoSuchKeyFoundException();
      }
    }
    if (this->lowOp == GT) {
      if (dataPage->keyArray[this->nextEntry] == traits::getLowBound(this)) {
        if (this->nextEntry + 1 == traits::LEAFSIZE) {
          this->nextEntry = 0;
          this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
          this->currentPageNum = dataPage->rightSibPageNo;
          this->bufMgr->readPage(this->file, this->currentPageNum, this->currentPageData);
        } else this->nextEntry++;
      }
    }
    if (dataPage->keyArray[this->nextEntry] > traits::getUpperBound(this)) {
      this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
      throw NoSuchKeyFoundException();
    }
    else if (this->highOp == LT && dataPage->keyArray[this->nextEntry] == traits::getUpperBound(this)){
      this->bufMgr->unPinPage(this->file, this->currentPageNum, false);
      throw NoSuchKeyFoundException();
    }
  } else {
    #ifdef DEBUG
    //assert(0);
    #endif
  }
}


template <typename keyType, class traits=BTreeIndex::keyTraits<keyType> >
const void BTreeIndex::insertKeyTemplate(const void* key, const RecordId rid) {
  typedef typename traits::nonLeafType nonLeafType;
  typedef typename traits::leafType leafType;
  Page* rootPage;
  keyType keyValue = *reinterpret_cast<keyType*>(const_cast<void*>(key));
  this->bufMgr->readPage(this->file, this->rootPageNum, rootPage);
  nonLeafType* rootData = reinterpret_cast<nonLeafType*>(rootPage);
  if (rootData->pageNoArray[0] == Page::INVALID_NUMBER) {
    Page* lessKey, *greaterKey;
    this->bufMgr->allocPage(this->file, rootData->pageNoArray[0], lessKey);
    this->bufMgr->allocPage(this->file, rootData->pageNoArray[1], greaterKey);

    leafType dataPageLeft, dataPageRight;
    memset(&dataPageLeft, 0, sizeof(leafType));
    memset(&dataPageRight, 0, sizeof(leafType));

    dataPageLeft.rightSibPageNo = rootData->pageNoArray[1];

    memcpy(lessKey, &dataPageLeft, sizeof(leafType));
    this->bufMgr->unPinPage(this->file, rootData->pageNoArray[0], true);

    dataPageRight.keyArray[0] = keyValue;
    dataPageRight.ridArray[0] = rid;
    memcpy(greaterKey, &dataPageRight, sizeof(leafType));
    this->bufMgr->unPinPage(this->file, rootData->pageNoArray[1], true);

    rootData->level = 2;
    rootData->keyArray[0] = keyValue;
    this->bufMgr->unPinPage(this->file, this->rootPageNum, true);
  } else {
    PageId dataPageNum;
    PageId dataPageNumPrev;
    Page* tempPage;
    int insertAt = -1, endOfRecordsOffset;
    getPageNoAndOffsetOfKeyInsert<keyType>(key, rootPage, dataPageNum, insertAt, endOfRecordsOffset, dataPageNumPrev);
    this->bufMgr->readPage(this->file, dataPageNum, tempPage);
    leafType* dataPage = reinterpret_cast<leafType*>(tempPage);

    for (int j = endOfRecordsOffset; j > insertAt; j--) {
      dataPage->ridArray[j] = dataPage->ridArray[j-1];
      dataPage->keyArray[j] = dataPage->keyArray[j-1];
    }
    dataPage->ridArray[insertAt] = rid;
    dataPage->keyArray[insertAt] = keyValue;
    this->bufMgr->unPinPage(this->file, dataPageNum, true);
#ifdef DEBUG
    cout << "DBG: Key " << keyValue << " inserted on page " << dataPageNum << " at offset " << insertAt << ":" << endOfRecordsOffset << endl;
#endif
  }
}

}
