#ifndef CPPJIEBA_QUERYSEGMENT_H
#define CPPJIEBA_QUERYSEGMENT_H

#include <algorithm>
#include <set>
#include <cassert>
#include <vector>
#include "limonp/Logging.hpp"
#include "DictTrie.hpp"
#include "SegmentBase.hpp"
#include "FullSegment.hpp"
#include "MixSegment.hpp"
#include "Unicode.hpp"

namespace cppjieba {

struct WordRangeWithOffset
{
  size_t offset;
  size_t len;
};

class QuerySegment: public SegmentBase {
 public:
  QuerySegment(const string& dict, const string& model, const string& userDict = "")
    : mixSeg_(dict, model, userDict),
      trie_(mixSeg_.GetDictTrie()) {
  }
  QuerySegment(const DictTrie* dictTrie, const HMMModel* model)
    : mixSeg_(dictTrie, model), trie_(dictTrie) {
  }
  ~QuerySegment() {
  }

  void Cut(const string& sentence, vector<string>& words) const {
    Cut(sentence, words, true);
  }
  void Cut(const string& sentence, vector<string>& words, bool hmm) const {
    vector<Word> tmp;
    Cut(sentence, tmp, hmm);
    GetStringsFromWords(tmp, words);
  }
  void Cut(const string& sentence, vector<Word>& words, bool hmm = true) const {
    PreFilter pre_filter(symbols_, sentence);
    PreFilter::Range range;
    vector<WordRange> wrs;
    wrs.reserve(sentence.size()/2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      Cut(range.begin, range.end, wrs, hmm);
    }
    words.clear();
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
  }

  void Cut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res, bool hmm) const {
    //use mix Cut first
    vector<WordRange> mixRes;
    mixSeg_.Cut(begin, end, mixRes, hmm);

    vector<WordRange> fullRes;
    for (vector<WordRange>::const_iterator mixResItr = mixRes.begin(); mixResItr != mixRes.end(); mixResItr++) {
      if (mixResItr->Length() > 2) {
        for (size_t i = 0; i + 1 < mixResItr->Length(); i++) {
          WordRange wr(mixResItr->left + i, mixResItr->left + i + 1);
          if (trie_->Find(wr.left, wr.right + 1) != NULL) {
            res.push_back(wr);
          }
        }
      }
      if (mixResItr->Length() > 3) {
        for (size_t i = 0; i + 2 < mixResItr->Length(); i++) {
          WordRange wr(mixResItr->left + i, mixResItr->left + i + 2);
          if (trie_->Find(wr.left, wr.right + 1) != NULL) {
            res.push_back(wr);
          }
        }
      }
      res.push_back(*mixResItr);
    }
  }

  inline void getOffsetsWithRanges(const vector<WordRange> & word_ranges, vector<WordRangeWithOffset> & words_with_offset)
  { 
    words_with_offset.resize(word_ranges.size());

    for(size_t i = 0 ; i < word_ranges.size() ; i++)
    {
      assert(word_ranges[i].left->offset <= word_ranges[i].right->offset);

      words_with_offset[i].offset = word_ranges[i].left->offset;
      words_with_offset[i].len = word_ranges[i].right->offset - word_ranges[i].left->offset + word_ranges[i].right->len;
    }
  }
  
  void cutWithStringRange(const string& sentence, vector<WordRangeWithOffset> & words_with_offset, bool hmm = true)
  {
    PreFilter pre_filter(symbols_, sentence);
    PreFilter::Range range;
    vector<WordRange> word_ranges;
    word_ranges.reserve(sentence.size()/2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      Cut(range.begin, range.end, word_ranges, hmm);
    }
    getOffsetsWithRanges(word_ranges, words_with_offset);
  }

 private:
  bool IsAllAscii(const Unicode& s) const {
   for(size_t i = 0; i < s.size(); i++) {
     if (s[i] >= 0x80) {
       return false;
     }
   }
   return true;
  }
  MixSegment mixSeg_;
  const DictTrie* trie_;
}; // QuerySegment

} // namespace cppjieba

#endif
