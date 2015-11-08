/*
  RcppEigenwords
  
  sentence の要素で-1となっているのは NULL word に対応する．
*/

#include <Rcpp.h>
#include <RcppEigen.h>
#include "redsvd.hpp"

// [[Rcpp::depends(RcppEigen)]]

using Eigen::MatrixXd;
using Eigen::MatrixXi;
using Eigen::VectorXd;
using Eigen::VectorXi;
using Eigen::MappedSparseMatrix;
typedef Eigen::Map<Eigen::VectorXi> MapIM;
typedef Eigen::MappedSparseMatrix<int, Eigen::RowMajor, std::ptrdiff_t> MapMatI;
typedef Eigen::SparseMatrix<double, Eigen::RowMajor, std::ptrdiff_t> dSparseMatrix;
typedef Eigen::SparseMatrix<int, Eigen::RowMajor, std::ptrdiff_t> iSparseMatrix;
typedef Eigen::Triplet<int> Triplet;

int TRIPLET_VECTOR_SIZE = 1000000;


// [[Rcpp::export]]
Rcpp::List EigenwordsRedSVD(MapIM& sentence, int window_size, int vocab_size,
                            int k, bool skip_null_words, bool mode_oscca) {
  
  unsigned long long i, j, j2;
  unsigned long long sentence_size = sentence.size();
  unsigned long long lr_col_size = (unsigned long long)window_size*(unsigned long long)vocab_size;
  unsigned long long c_col_size = 2*lr_col_size;
  unsigned long long n_pushed_triplets = 0;
  long long i_word1, i_word2;
  int i_offset1, i_offset2;
  int offsets[2*window_size];
  bool word_isnot_null_word;
  
  iSparseMatrix twc(vocab_size, c_col_size), twc_temp(vocab_size, c_col_size);
  iSparseMatrix tll(lr_col_size, lr_col_size), tll_temp(lr_col_size, lr_col_size);
  iSparseMatrix tlr(lr_col_size, lr_col_size), tlr_temp(lr_col_size, lr_col_size);
  iSparseMatrix trr(lr_col_size, lr_col_size), trr_temp(lr_col_size, lr_col_size);
  MatrixXd phi_l, phi_r;
  
  VectorXi tww_diag(vocab_size);
  VectorXi tcc_diag(c_col_size);
  tww_diag.setZero();  
  tcc_diag.setZero();
  
  std::vector<Triplet> twc_tripletList;
  std::vector<Triplet> tll_tripletList;
  std::vector<Triplet> tlr_tripletList;
  std::vector<Triplet> trr_tripletList;
  twc_tripletList.reserve(TRIPLET_VECTOR_SIZE);
  tll_tripletList.reserve(TRIPLET_VECTOR_SIZE);
  tlr_tripletList.reserve(TRIPLET_VECTOR_SIZE);
  trr_tripletList.reserve(TRIPLET_VECTOR_SIZE);
  

  std::cout << "mode          = ";
  if (mode_oscca) {
    std::cout << "OSCCA" << std::endl;
  } else {
    std::cout << "TSCCA" << std::endl;
  }
  std::cout << "window size   = " << window_size   << std::endl;
  std::cout << "vocab size    = " << vocab_size    << std::endl;
  std::cout << "sentence size = " << sentence_size << std::endl;
  std::cout << "c_col_size    = " << c_col_size    << std::endl;
  std::cout << std::endl;

  i_offset1 = 0;
  for (int offset=-window_size; offset<=window_size; offset++){
    if (offset != 0) {
      offsets[i_offset1] = offset;
      i_offset1++;
    }
  }
  
  for (unsigned long long i_sentence=0; i_sentence<sentence_size; i_sentence++) {
    word_isnot_null_word = sentence[i_sentence] >= 0;
    
    if (word_isnot_null_word) {
      i = sentence[i_sentence];
      tww_diag(i) += 1;
    }
    
    for (i_offset1=0; i_offset1<2*window_size; i_offset1++) {
      i_word1 = i_sentence + offsets[i_offset1];
      if (i_word1 >= 0 && i_word1 < sentence_size && sentence[i_word1] >= 0) {
        j = sentence[i_word1] + vocab_size * i_offset1;

        // Skip if sentence[i_sentence] is null words and skip_null_words is true
        if (word_isnot_null_word || !skip_null_words){
          
          if (mode_oscca) {
            tcc_diag(j) += 1;
          } else {
            
            for (i_offset2=0; i_offset2<2*window_size; i_offset2++) {
              i_word2 = i_sentence + offsets[i_offset2];
              
              if (i_word2 >= 0 && i_word2 < sentence_size && sentence[i_word2] >= 0) {
                j2 = sentence[i_word2] + vocab_size * i_offset2;
                
                if (j < lr_col_size) {
                  if (j2 < lr_col_size) {
                    tll_tripletList.push_back(Triplet(j, j2, 1));
                  } else {
                    tlr_tripletList.push_back(Triplet(j, j2 - lr_col_size, 1));
                  }
                } else {
                  if (j2 >= lr_col_size) {
                    trr_tripletList.push_back(Triplet(j - lr_col_size, j2 - lr_col_size, 1));
                  }
                }
              }
            }
            
          }
        }
        
        if (word_isnot_null_word) {
          twc_tripletList.push_back(Triplet(i, j, 1));
        }
      }
    }
    
    n_pushed_triplets++;
    
    if (n_pushed_triplets >= TRIPLET_VECTOR_SIZE - 3*window_size || i_sentence == sentence_size - 1) {
      twc_temp.setFromTriplets(twc_tripletList.begin(), twc_tripletList.end());
      twc_tripletList.clear();
      twc += twc_temp;
      twc_temp.setZero();
      
      if (!mode_oscca) {
        tll_temp.setFromTriplets(tll_tripletList.begin(), tll_tripletList.end());
        tll_tripletList.clear();
        tll += tll_temp;
        tll_temp.setZero();
        
        tlr_temp.setFromTriplets(tlr_tripletList.begin(), tlr_tripletList.end());
        tlr_tripletList.clear();
        tlr += tlr_temp;
        tlr_temp.setZero();
        
        trr_temp.setFromTriplets(trr_tripletList.begin(), trr_tripletList.end());
        trr_tripletList.clear();
        trr += trr_temp;
        trr_temp.setZero();
      }
      
      n_pushed_triplets = 0;
    }
  }
  
  
  if (mode_oscca) {
    // One Step CCA
    
    std::cout << "Calculate OSCCA..." << std::endl;
    std::cout << "Density of twc = " << twc.nonZeros() << "/" << twc.rows() * twc.cols() << std::endl;

    VectorXd tww_h(tww_diag.cast <double> ().cwiseInverse().cwiseSqrt());
    VectorXd tcc_h(tcc_diag.cast <double> ().cwiseInverse().cwiseSqrt());
    dSparseMatrix a(tww_h.asDiagonal() * (twc.cast <double> ().eval()) * tcc_h.asDiagonal());
  
    std::cout << "Calculate Randomized SVD..." << std::endl;
    RedSVD::RedSVD<dSparseMatrix> svdA(a, k);
    
    return Rcpp::List::create(
      Rcpp::Named("tWC") = Rcpp::wrap(twc.cast <double> ()),
      Rcpp::Named("tWW_h") = Rcpp::wrap(tww_h),
      Rcpp::Named("tCC_h") = Rcpp::wrap(tcc_h),
      Rcpp::Named("A") = Rcpp::wrap(a),
      Rcpp::Named("V") = Rcpp::wrap(svdA.matrixV()),
      Rcpp::Named("U") = Rcpp::wrap(svdA.matrixU()),
      Rcpp::Named("D") = Rcpp::wrap(svdA.singularValues()),
      Rcpp::Named("window.size") = Rcpp::wrap(window_size),
      Rcpp::Named("vocab.size") = Rcpp::wrap(vocab_size),
      Rcpp::Named("skip.null.words") = Rcpp::wrap(skip_null_words),
      Rcpp::Named("k") = Rcpp::wrap(k)
    );
    
  } else {
    // Two Step CCA
    
    tll.makeCompressed();
    tlr.makeCompressed();
    trr.makeCompressed();

    std::cout << "Density of twc = " << twc.nonZeros() << "/" << twc.rows() * twc.cols() << std::endl;
    std::cout << "Density of tll = " << tll.nonZeros() << "/" << tll.rows() * tll.cols() << std::endl;
    std::cout << "Density of tlr = " << tlr.nonZeros() << "/" << tlr.rows() * tlr.cols() << std::endl;
    std::cout << "Density of trr = " << trr.nonZeros() << "/" << trr.rows() * trr.cols() << std::endl;
    std::cout << "Calculate TSCCA..." << std::endl;
    
    // Two Step CCA : Step 1
    VectorXd tll_h(tll.diagonal().cast <double> ().cwiseInverse().cwiseSqrt());
    VectorXd trr_h(trr.diagonal().cast <double> ().cwiseInverse().cwiseSqrt());
    dSparseMatrix b(tll_h.asDiagonal() * (tlr.cast <double> ().eval()) * trr_h.asDiagonal());
    
    std::cout << "Calculate Randomized SVD (1/2)..." << std::endl;
    RedSVD::RedSVD<dSparseMatrix> svdB(b, k);
    b.resize(0, 0);
    
    // Two Step CCA : Step 2
    phi_l = svdB.matrixU();
    phi_r = svdB.matrixV();
        
    VectorXd tww_h(tww_diag.cast <double> ().cwiseInverse().cwiseSqrt());
    VectorXd tss_h1((phi_l.transpose() * tll.cast <double> () * phi_l).eval().diagonal().cwiseInverse().cwiseSqrt());
    VectorXd tss_h2((phi_r.transpose() * trr.cast <double> () * phi_r).eval().diagonal().cwiseInverse().cwiseSqrt());
    VectorXd tss_h(2*k);
    tss_h << tss_h1, tss_h2;
    MatrixXd tws(vocab_size, 2*k);
    tws << twc.topLeftCorner(vocab_size, c_col_size/2).cast <double> () * phi_l, twc.topRightCorner(vocab_size, c_col_size/2).cast <double> () * phi_r;
    MatrixXd a(tww_h.asDiagonal() * tws * tss_h.asDiagonal());

    std::cout << "Calculate Randomized SVD (2/2)..." << std::endl;
    RedSVD::RedSVD<MatrixXd> svdA(a, k);
    
    return Rcpp::List::create(
      Rcpp::Named("tWS") = Rcpp::wrap(tws),
      Rcpp::Named("tWW_h") = Rcpp::wrap(tww_h),
      Rcpp::Named("tSS_h") = Rcpp::wrap(tss_h),
      Rcpp::Named("A") = Rcpp::wrap(a),
      Rcpp::Named("V") = Rcpp::wrap(svdA.matrixV()),
      Rcpp::Named("U") = Rcpp::wrap(svdA.matrixU()),
      Rcpp::Named("D") = Rcpp::wrap(svdA.singularValues()),
      Rcpp::Named("window.size") = Rcpp::wrap(window_size),
      Rcpp::Named("vocab.size") = Rcpp::wrap(vocab_size),
      Rcpp::Named("skip.null.words") = Rcpp::wrap(skip_null_words),
      Rcpp::Named("k") = Rcpp::wrap(k)
    );
  }
}
