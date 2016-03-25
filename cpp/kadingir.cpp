/* kadingir.cpp
 *
 * Example: make && ./kadingir ../data/text8 output.txt
 */

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include "../src/kadingir_core.hpp"


typedef std::map<std::string, int> MapCounter;
typedef std::pair<std::string, int> PairCounter;
typedef std::vector<PairCounter>::const_iterator PairIterator;

bool sort_greater(const PairCounter& left, const PairCounter& right)
{
  return left.second > right.second;
}


int main(int argc, char* argv[])
{
  unsigned long long n_tokens = 0;
  unsigned long long n_documents = 0;
  char ch;
  std::string word_temp;
  /* TODO: コマンドライン引数でいい感じに書きたい */
  const char *file_path = argv[1];
  const char *path_output = argv[2];
  const int n_vocab = 10000;
  const int dim = 50;
  const int window = 2;
  const bool debug = false;
  MapCounter count_table, table_wordtype_id;
  
  std::ifstream fin;
  fin.unsetf(std::ios::skipws);
  fin.open(file_path);
  
  while (!fin.eof()) {
    fin >> ch;
    if (fin.eof()) break;
    
    if (ch == '\n' || ch == ' ') {
      // If `ch` is a separation of documents or words
      
      n_tokens += 1;

      if (ch == '\n') {
        n_documents += 1;
      }
      
      if (!word_temp.empty()) {
        if (count_table.count(word_temp) == 0) {
          count_table.insert(PairCounter(word_temp, 1));
        } else {
          count_table[word_temp] += 1;
        }
        word_temp.erase();
      }
      
      
    } else {
      // If `ch` is a character of a word
      word_temp += ch;
    }      
  }
  fin.close();
  
  // Sort `count_table`.
  std::vector<PairCounter> count_vector(count_table.begin(), count_table.end());  
  std::sort(count_vector.begin(), count_vector.end(), sort_greater);
  
  // Construct table (word -> wordtype id)
  unsigned long long i_vocab = 1;
  for (PairIterator iter = count_vector.begin(); iter != count_vector.end(); iter++) {
    std::string iter_str = iter->first;
    int iter_int = iter->second;
    
    table_wordtype_id.insert(PairCounter(iter_str, i_vocab));

    i_vocab++;

    if (i_vocab >= n_vocab) {
      std::cout << iter_int << " " << iter_str << std::endl;
      break;
    }
  }

  // Convert words to wordtype id
  unsigned long long i_tokens = 0, n_oov = 0;
  std::vector<int> tokens(n_tokens);

  fin.open(file_path);

  while (!fin.eof()) {
    fin >> ch;
    if (fin.eof()) break;

    if (ch == '\n' || ch == ' ') {
      // If `ch` is a separation of documents or words
      if (!word_temp.empty()) {
        if (table_wordtype_id.count(word_temp) == 0) {
          // If the token is Out of Vocabulary
          tokens[i_tokens] = 0;
          n_oov += 1;
        } else {
          // Otherwise
          tokens[i_tokens] = table_wordtype_id[word_temp];
        }

        i_tokens += 1;
        word_temp.erase();
      }
    } else {
      // If `ch` is a character of a word
      word_temp += ch;
    }  
  }

  // Display some informations
  std::cout << "Path        : " << file_path << std::endl;
  std::cout << "# of tokens : " << n_tokens << std::endl;
  std::cout << "# of OOV    : " << n_oov << std::endl;
  std::cout << "# of vocab  : " << n_vocab << std::endl;
  std::cout << "Coverage(%) : " << 100 * (n_tokens - n_oov) / (double)n_tokens << std::endl;

  // Execute EigenwordsOSCCA
  EigenwordsOSCCA eigenwords(tokens, window, n_vocab, dim, debug);
  eigenwords.compute();
  MatrixXd vectors = eigenwords.get_word_vectors();

  // Output vector representations as a txt file
  std::ofstream file_output;
  file_output.open(path_output, std::ios::out);

  for (int i = 0; i < vectors.rows(); i++) {
    if (i == 0) {
      file_output << "<OOV> ";
    } else {
      file_output << count_vector[i - 1].first << " ";
    }

    for (int j = 0; j < vectors.cols(); j++) {
      file_output << vectors(i, j) << " ";
    }
    file_output << std::endl;
  }

  return 0;
}
