#ifndef VIENNA_RNA_PACKAGE_GQUAD_H
#define VIENNA_RNA_PACKAGE_GQUAD_H

#include <string.h>

#include <ViennaRNA/datastructures/basic.h>
#include "ViennaRNA/datastructures/sparse_mx.h"
#include <ViennaRNA/utils/log.h>
#include <ViennaRNA/alphabet.h>
#include <ViennaRNA/fold_compound.h>
#include <ViennaRNA/params/basic.h>

#ifndef INLINE
#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif
#endif

#ifdef VRNA_WARN_DEPRECATED
# if defined(__clang__)
#  define DEPRECATED(func, msg) func __attribute__ ((deprecated("", msg)))
# elif defined(__GNUC__)
#  define DEPRECATED(func, msg) func __attribute__ ((deprecated(msg)))
# else
#  define DEPRECATED(func, msg) func
# endif
#else
# define DEPRECATED(func, msg) func
#endif

/**
 *  @file       gquad.h
 *  @ingroup    paired_modules
 *  @brief      G-quadruplexes
 */

/**
 *  @addtogroup gquads
 *  @{
 *
 *  @brief Various functions related to G-quadruplex computations
 */


int         E_gquad(int           L,
                    int           l[3],
                    vrna_param_t  *P);


FLT_OR_DBL  exp_E_gquad(int               L,
                        int               l[3],
                        vrna_exp_param_t  *pf);


void E_gquad_ali_en(int           i,
                    int           L,
                    int           l[3],
                    const short   **S,
                    unsigned int  **a2s,
                    unsigned int  n_seq,
                    vrna_param_t  *P,
                    int           en[2]);



vrna_smx_csr(int) *
vrna_gq_pos_mfe(vrna_fold_compound_t *fc);

vrna_smx_csr(FLT_OR_DBL) *
vrna_gq_pos_pf(vrna_fold_compound_t *fc);


int **get_gquad_L_matrix(short        *S,
                         int          start,
                         int          maxdist,
                         int          n,
                         int          **g,
                         vrna_param_t *P);


void
vrna_gquad_mx_local_update(vrna_fold_compound_t *fc,
                           int                  start);


void get_gquad_pattern_mfe(short        *S,
                           int          i,
                           int          j,
                           vrna_param_t *P,
                           int          *L,
                           int          l[3]);


void
get_gquad_pattern_exhaustive(short        *S,
                             int          i,
                             int          j,
                             vrna_param_t *P,
                             int          *L,
                             int          *l,
                             int          threshold);


void get_gquad_pattern_pf(short             *S,
                          int               i,
                          int               j,
                          vrna_exp_param_t  *pf,
                          int               *L,
                          int               l[3]);


plist *get_plist_gquad_from_pr(short            *S,
                               int              gi,
                               int              gj,
                               vrna_smx_csr(FLT_OR_DBL)  *q_gq,
                               FLT_OR_DBL       *probs,
                               FLT_OR_DBL       *scale,
                               vrna_exp_param_t *pf);

vrna_ep_t *
vrna_plist_gquad_from_pr(vrna_fold_compound_t *fc,
                         int               gi,
                         int               gj);


plist *get_plist_gquad_from_pr_max(short            *S,
                                   int              gi,
                                   int              gj,
                                   vrna_smx_csr(FLT_OR_DBL)  *q_gq,
                                   FLT_OR_DBL       *probs,
                                   FLT_OR_DBL       *scale,
                                   int              *L,
                                   int              l[3],
                                   vrna_exp_param_t *pf);


plist *get_plist_gquad_from_db(const char *structure,
                               float      pr);


vrna_ep_t *
vrna_plist_gquad_from_pr_max(vrna_fold_compound_t *fc,
                             int                  gi,
                             int                  gj,
                             int                  *Lmax,
                             int                  lmax[3]);


int         get_gquad_count(short *S,
                            int   i,
                            int   j);


int         get_gquad_layer_count(short *S,
                                  int   i,
                                  int   j);


void get_gquad_pattern_mfe_ali(short        **S,
                               unsigned int **a2s,
                               short        *S_cons,
                               int          n_seq,
                               int          i,
                               int          j,
                               vrna_param_t *P,
                               int          *L,
                               int          l[3]);


/**
 *  given a dot-bracket structure (possibly) containing gquads encoded
 *  by '+' signs, find first gquad, return end position or 0 if none found
 *  Upon return L and l[] contain the number of stacked layers, as well as
 *  the lengths of the linker regions.
 *  To parse a string with many gquads, call parse_gquad repeatedly e.g.
 *  end1 = parse_gquad(struc, &L, l); ... ;
 *  end2 = parse_gquad(struc+end1, &L, l); end2+=end1; ... ;
 *  end3 = parse_gquad(struc+end2, &L, l); end3+=end2; ... ;
 */
int parse_gquad(const char  *struc,
                int         *L,
                int         l[3]);




int backtrack_GQuad_IntLoop_L(int          c,
                                             int          i,
                                             int          j,
                                             int          type,
                                             short        *S,
                                             int          **ggg,
                                             int          maxdist,
                                             int          *p,
                                             int          *q,
                                             vrna_param_t *P);


int
vrna_BT_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bp_stack_t       *bp_stack,
                  unsigned int          *stack_count);


int
vrna_bt_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bps_t            bp_stack);


int
vrna_bt_gquad_mfe(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  vrna_bps_t            bp_stack);


int
vrna_BT_gquad_mfe(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  vrna_bp_stack_t       *bp_stack,
                  unsigned int          *stack_count);


int
vrna_bt_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bps_t            bp_stack);


int
vrna_BT_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bp_stack_t       *bp_stack,
                  unsigned int          *stack_count);


/**
 *  backtrack an interior loop like enclosed g-quadruplex
 *  with closing pair (i,j) with underlying Lfold matrix
 *
 *  @param c      The total contribution the loop should resemble
 *  @param i      position i of enclosing pair
 *  @param j      position j of enclosing pair
 *  @param type   base pair type of enclosing pair (must be reverse type)
 *  @param S      integer encoded sequence
 *  @param ggg    triangular matrix containing g-quadruplex contributions
 *  @param p      here the 5' position of the gquad is stored
 *  @param q      here the 3' position of the gquad is stored
 *  @param P      the datastructure containing the precalculated contibutions
 *
 *  @return       1 on success, 0 if no gquad found
 */
int
backtrack_GQuad_IntLoop_L(int           c,
                          int           i,
                          int           j,
                          int           type,
                          short         *S,
                          int           **ggg,
                          int           maxdist,
                          int           *p,
                          int           *q,
                          vrna_param_t  *P);


int
backtrack_GQuad_IntLoop_L_comparative(int           c,
                                      int           i,
                                      int           j,
                                      unsigned int  *type,
                                      short         *S_cons,
                                      short         **S5,
                                      short         **S3,
                                      unsigned int  **a2s,
                                      int           **ggg,
                                      int           *p,
                                      int           *q,
                                      int           n_seq,
                                      vrna_param_t  *P);


int
vrna_E_gq_intLoop(vrna_fold_compound_t *fc,
                  int           i,
                  int           j);


int
E_GQuad_IntLoop_L_comparative(int           i,
                              int           j,
                              unsigned int  *tt,
                              short         *S_cons,
                              short         **S5,
                              short         **S3,
                              unsigned int  **a2s,
                              int           **ggg,
                              int           n_seq,
                              vrna_param_t  *P);


int *
vrna_E_gq_intLoop_exhaustive(vrna_fold_compound_t *fc,
                            int          i,
                            int          j,
                           int          **p_p,
                           int          **q_p,
                           int          threshold);


int
E_GQuad_IntLoop_L(int           i,
                  int           j,
                  int           type,
                  short         *S,
                  int           **ggg,
                  int           maxdist,
                  vrna_param_t  *P);


FLT_OR_DBL
vrna_exp_E_gq_intLoop(vrna_fold_compound_t *fc,
                      int               i,
                      int               j);


/**
 * @}
 */

#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY

/**
 *  @brief Get a triangular matrix prefilled with minimum free energy
 *  contributions of G-quadruplexes.
 *
 *  At each position ij in the matrix, the minimum free energy of any
 *  G-quadruplex delimited by i and j is stored. If no G-quadruplex formation
 *  is possible, the matrix element is set to INF.
 *  Access the elements in the matrix via matrix[indx[j]+i]. To get
 *  the integer array indx see get_jindx().
 *
 *  @see get_jindx(), encode_sequence()
 *
 *  @param S  The encoded sequence
 *  @param P  A pointer to the data structure containing the precomputed energy contributions
 *  @return   A pointer to the G-quadruplex contribution matrix
 */
DEPRECATED(int *get_gquad_matrix(short *S, vrna_param_t *P),
           "Use vrna_gq_pos_mfe() instead");

DEPRECATED(int *get_gquad_ali_matrix(unsigned int  n,
                          short         *S_cons,
                          short         **S,
                          unsigned int  **a2s,
                          int           n_seq,
                          vrna_param_t  *P),
           "Use vrna_gq_pos_mfe() instead");


DEPRECATED(FLT_OR_DBL *get_gquad_pf_matrix(short             *S,
                                FLT_OR_DBL        *scale,
                                vrna_exp_param_t  *pf),
           "Use vrna_gq_pos_pf() instead");


DEPRECATED(FLT_OR_DBL *get_gquad_pf_matrix_comparative(unsigned int  n,
                                            short             *S_cons,
                                            short             **S,
                                            unsigned int      **a2s,
                                            FLT_OR_DBL        *scale,
                                            unsigned int      n_seq,
                                            vrna_exp_param_t  *pf),
           "Use vrna_gq_pos_pf() instead");


#endif

#endif