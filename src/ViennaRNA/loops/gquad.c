/*
 * gquad.c
 *
 * Ronny Lorenz 2012
 *
 * ViennaRNA Package
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/datastructures/basic.h"
#include "ViennaRNA/params/constants.h"
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/utils/alignments.h"
#include "ViennaRNA/utils/log.h"
#include "ViennaRNA/loops/gquad.h"

#ifndef INLINE
#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif
#endif

/**
 *  Use this macro to loop over each G-quadruplex
 *  delimited by a and b within the subsequence [c,d]
 */
#define FOR_EACH_GQUAD(a, b, c, d)  \
  for ((a) = (d) - VRNA_GQUAD_MIN_BOX_SIZE + 1; (a) >= (c); (a)--) \
    for ((b) = (a) + VRNA_GQUAD_MIN_BOX_SIZE - 1; \
         (b) <= MIN2((d), (a) + VRNA_GQUAD_MAX_BOX_SIZE - 1); \
         (b)++)

#define FOR_EACH_GQUAD_INC(a, b, c, d)  \
  for ((a) = (c); (a) <= (d) - VRNA_GQUAD_MIN_BOX_SIZE + 1; (a)++) \
    for ((b) = (a) + VRNA_GQUAD_MIN_BOX_SIZE - 1; \
         (b) <= MIN2((d), (a) + VRNA_GQUAD_MAX_BOX_SIZE - 1); \
         (b)++)

/**
 *  This macro does almost the same as FOR_EACH_GQUAD() but keeps
 *  the 5' delimiter fixed. 'b' is the 3' delimiter of the gquad,
 *  for gquads within subsequence [a,c] that have 5' delimiter 'a'
 */
#define FOR_EACH_GQUAD_AT(a, b, c)  \
  for ((b) = (a) + VRNA_GQUAD_MIN_BOX_SIZE - 1; \
       (b) <= MIN2((c), (a) + VRNA_GQUAD_MAX_BOX_SIZE - 1); \
       (b)++)


struct gquad_ali_helper {
  short             **S;
  unsigned int      **a2s;
  int               n_seq;
  vrna_param_t      *P;
  vrna_exp_param_t  *pf;
  int               L;
  int               *l;
};

/*
 #################################
 # PRIVATE FUNCTION DECLARATIONS #
 #################################
 */

PRIVATE INLINE
int *
get_g_islands(short *S);


PRIVATE INLINE
int *
get_g_islands_sub(short *S,
                  int   i,
                  int   j);


/**
 *  IMPORTANT:
 *  If you don't know how to use this function, DONT'T USE IT!
 *
 *  The function pointer this function takes as argument is
 *  used for individual calculations with each g-quadruplex
 *  delimited by [i,j].
 *  The function it points to always receives as first 3 arguments
 *  position i, the stack size L and an array l[3] containing the
 *  individual linker sizes.
 *  The remaining 4 (void *) pointers of the callback function receive
 *  the parameters 'data', 'P', 'aux1' and 'aux2' and thus may be
 *  used to pass whatever data you like to.
 *  As the names of those parameters suggest the convention is that
 *  'data' should be used as a pointer where data is stored into,
 *  e.g the MFE or PF and the 'P' parameter should actually be a
 *  'vrna_param_t *' or 'vrna_exp_param_t *' type.
 *  However, what you actually pass obviously depends on the
 *  function the pointer is pointing to.
 *
 *  Although all of this may look like an overkill, it is found
 *  to be almost as fast as implementing g-quadruplex enumeration
 *  in each individual scenario, i.e. code duplication.
 *  Using this function, however, ensures that all g-quadruplex
 *  enumerations are absolutely identical.
 */
PRIVATE
void
process_gquad_enumeration(int *gg,
                          int i,
                          int j,
                          void ( *f )(int, int, int *,
                                      void *, void *, void *, void *),
                          void *data,
                          void *P,
                          void *aux1,
                          void *aux2);


/**
 *  MFE callback for process_gquad_enumeration()
 */
PRIVATE
void
gquad_mfe(int   i,
          int   L,
          int   *l,
          void  *data,
          void  *P,
          void  *NA,
          void  *NA2);


PRIVATE
void
gquad_mfe_pos(int   i,
              int   L,
              int   *l,
              void  *data,
              void  *P,
              void  *Lmfe,
              void  *lmfe);


PRIVATE void
gquad_mfe_ali_pos(int   i,
                  int   L,
                  int   *l,
                  void  *data,
                  void  *helper,
                  void  *Lmfe,
                  void  *lmfe);


PRIVATE
void
gquad_pos_exhaustive(int  i,
                     int  L,
                     int  *l,
                     void *data,
                     void *P,
                     void *Lex,
                     void *lex);


/**
 * Partition function callback for process_gquad_enumeration()
 */
PRIVATE
void
gquad_pf(int  i,
         int  L,
         int  *l,
         void *data,
         void *P,
         void *NA,
         void *NA2);


PRIVATE void
gquad_pf_ali(int  i,
             int  L,
             int  *l,
             void *data,
             void *helper,
             void *NA,
             void *NA2);


/**
 * Partition function callback for process_gquad_enumeration()
 * in contrast to gquad_pf() it stores the stack size L and
 * the linker lengths l[3] of the g-quadruplex that dominates
 * the interval [i,j]
 * (FLT_OR_DBL *)data must be 0. on entry
 */
PRIVATE
void
gquad_pf_pos(int  i,
             int  L,
             int  *l,
             void *data,
             void *pf,
             void *Lmax,
             void *lmax);


PRIVATE void
gquad_pf_pos_ali(int  i,
                 int  L,
                 int  *l,
                 void *data,
                 void *helper,
                 void *NA1,
                 void *NA2);


/**
 * MFE (alifold) callback for process_gquad_enumeration()
 */
PRIVATE
void
gquad_mfe_ali(int   i,
              int   L,
              int   *l,
              void  *data,
              void  *helper,
              void  *NA,
              void  *NA2);


/**
 * MFE (alifold) callback for process_gquad_enumeration()
 * with seperation of free energy and penalty contribution
 */
PRIVATE
void
gquad_mfe_ali_en(int  i,
                 int  L,
                 int  *l,
                 void *data,
                 void *helper,
                 void *NA,
                 void *NA2);


PRIVATE
void
gquad_interact(int  i,
               int  L,
               int  *l,
               void *data,
               void *pf,
               void *index,
               void *NA2);


PRIVATE
void
gquad_interact_ali(int  i,
                   int  L,
                   int  *l,
                   void *data,
                   void *index,
                   void *helper,
                   void *NA);


PRIVATE
void
gquad_count(int   i,
            int   L,
            int   *l,
            void  *data,
            void  *NA,
            void  *NA2,
            void  *NA3);


PRIVATE
void
gquad_count_layers(int  i,
                   int  L,
                   int  *l,
                   void *data,
                   void *NA,
                   void *NA2,
                   void *NA3);


/* other useful static functions */

PRIVATE
int
E_gquad_ali_penalty(int           i,
                    int           L,
                    int           l[3],
                    const short   **S,
                    unsigned int  n_seq,
                    vrna_param_t  *P);


PRIVATE
FLT_OR_DBL
exp_E_gquad_ali_penalty(int               i,
                        int               L,
                        int               l[3],
                        const short       **S,
                        unsigned int      n_seq,
                        vrna_exp_param_t  *P);


PRIVATE void
count_gquad_layer_mismatches(int          i,
                             int          L,
                             int          l[3],
                             const short  **S,
                             unsigned int n_seq,
                             unsigned int mm[2]);


PRIVATE int **
create_L_matrix(short         *S,
                int           start,
                int           maxdist,
                int           n,
                int           **g,
                vrna_param_t  *P);


PRIVATE int **
create_aliL_matrix(int          start,
                   int          maxdist,
                   int          n,
                   int          **g,
                   short        *S_cons,
                   short        **S,
                   unsigned int **a2s,
                   int          n_seq,
                   vrna_param_t *P);


PRIVATE void
gquad_mfe_ali_pos(int   i,
                  int   L,
                  int   *l,
                  void  *data,
                  void  *P,
                  void  *Lmfe,
                  void  *lmfe);


/*
 #########################################
 # BEGIN OF PUBLIC FUNCTION DEFINITIONS  #
 #      (all available in RNAlib)        #
 #########################################
 */

PUBLIC int
vrna_bt_gquad_mfe(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  vrna_bps_t            bp_stack)
{
  /*
   * here we do some fancy stuff to backtrace the stacksize and linker lengths
   * of the g-quadruplex that should reside within position i,j
   */
  short         *S, *S_enc, *S_tmp;
  unsigned int  n, n2;
  int           l[3], L, a, n_seq;
  vrna_param_t  *P;

  if (fc) {
    n = fc->length;
    P = fc->params;
    L = -1;
    S_tmp = NULL;

    switch (fc->type) {
      case VRNA_FC_TYPE_SINGLE:
        S_enc     = fc->sequence_encoding2;
        break;

      case VRNA_FC_TYPE_COMPARATIVE:
        S_enc         = fc->S_cons;
        break;
    }

    if (P->model_details.circ) {
      n2 = MIN2(n, VRNA_GQUAD_MAX_BOX_SIZE) - 1;

      S_tmp = (short *)vrna_alloc(sizeof(short) * (n + n2 + 1));
      memcpy(S_tmp, S_enc, sizeof(short) * (n + 1));
      memcpy(S_tmp + (n + 1), S_enc + 1, sizeof(short) * n2);
      S_tmp[0] = n + n2;
      S_enc = S_tmp;
      if (j < i)
        j += n;
    }
    vrna_log_debug("try bt gquad [%d,%d]", i, j);
    switch (fc->type) {
      case VRNA_FC_TYPE_SINGLE:
        get_gquad_pattern_mfe(S_enc, i, j, P, &L, l);
        break;

      case VRNA_FC_TYPE_COMPARATIVE:
        get_gquad_pattern_mfe_ali(fc->S, fc->a2s, fc->S_cons, fc->n_seq, i, j, P, &L, l);
        break;
    }

    if (L != -1) {
      /* fill the G's of the quadruplex into base_pair2 */
      for (a = 0; a < L; a++) {
        int p1, p2, p3, p4;
        p1 = i + a;
        p2 = p1 + L + l[0];
        p3 = p2 + L + l[1];
        p4 = p3 + L + l[2];
        if (p1 > n) {
          p1 = ((p1 - 1) % n) + 1;
          p2 = ((p2 - 1) % n) + 1;
          p3 = ((p3 - 1) % n) + 1;
          p4 = ((p4 - 1) % n) + 1;
        } else if (p2 > n) {
          p2 = ((p2 - 1) % n) + 1;
          p3 = ((p3 - 1) % n) + 1;
          p4 = ((p4 - 1) % n) + 1;
        } else if (p3 > n) {
          p3 = ((p3 - 1) % n) + 1;
          p4 = ((p4 - 1) % n) + 1;
        } else if (p4 > n) {
          p4 = ((p4 - 1) % n) + 1;
        }

        vrna_bps_push(bp_stack,
                      (vrna_bp_t){
                        .i = p1,
                        .j = p1
                      });
        vrna_bps_push(bp_stack,
                      (vrna_bp_t){
                        .i = p2,
                        .j = p2
                      });
        vrna_bps_push(bp_stack,
                      (vrna_bp_t){
                        .i = p3,
                        .j = p3
                      });
        vrna_bps_push(bp_stack,
                      (vrna_bp_t){
                        .i = p4,
                        .j = p4
                      });
      }
      free(S_tmp);
      return 1;
    } else {
      free(S_tmp);
      return 0;
    }
  }

  return 0;
}


PUBLIC int
vrna_BT_gquad_mfe(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  vrna_bp_stack_t       *bp_stack,
                  unsigned int          *stack_count)
{
  int r = 0;

  if ((fc) &&
      (bp_stack) &&
      (stack_count)) {
    vrna_bps_t bps = vrna_bps_init(4);
    r = vrna_bt_gquad_mfe(fc, i, j, bps);

    while (vrna_bps_size(bps) > 0) {
      vrna_bp_t bp = vrna_bps_pop(bps);
      bp_stack[++(*stack_count)].i = bp.i;
      bp_stack[*stack_count].j = bp.j;
    }

    vrna_bps_free(bps);
  }

  return r;
}


PUBLIC int
vrna_bt_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bps_t            bp_stack)
{
  int           energy, e_gq, dangles, p, q, maxl, minl, c0, l1, u1, u2;
  unsigned char type;
  unsigned int  **a2s, n_seq, s;
  char          *ptype;
  short         si, sj, *S, *S1, **SS, **S5, **S3;
  vrna_smx_csr(int) *c_gq;

  vrna_param_t  *P;
  vrna_md_t     *md;

  n_seq   = fc->n_seq;
  S       = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;
  S1      = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding : fc->S_cons;
  SS      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  S5      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S5;
  S3      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S3;
  a2s     = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->a2s;
  c_gq    = fc->matrices->c_gq;
  P       = fc->params;
  md      = &(P->model_details);
  dangles = md->dangles;
  si      = S1[i + 1];
  sj      = S1[j - 1];
  energy  = 0;

  switch (fc->type) {
    case VRNA_FC_TYPE_SINGLE:
      type    = vrna_get_ptype_md(S[i], S[j], md);
      if (dangles == 2)
        energy += P->mismatchI[type][si][sj];

      if (type > 2)
        energy += P->TerminalAU;
      break;

    case VRNA_FC_TYPE_COMPARATIVE:
      for (s = 0; s < n_seq; s++) {
        type = vrna_get_ptype_md(SS[s][i], SS[s][j], md);
        if (md->dangles == 2)
          energy += P->mismatchI[type][S3[s][i]][S5[s][j]];

        if (type > 2)
          energy += P->TerminalAU;
      }
      break;

    default:
      return INF;
  }

  p = i + 1;
  if (S1[p] == 3) {
    if (p < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minl  = j - i + p - MAXLOOP - 2;
      c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minl  = MAX2(c0, minl);
      c0    = j - 3;
      maxl  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxl  = MIN2(c0, maxl);
      for (q = minl; q < maxl; q++) {
        if (S1[q] != 3)
          continue;
#ifndef VRNA_DISABLE_C11_FEATURES
        e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
        e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

        if (e_gq != INF) {
          c0 = energy + e_gq;

          switch (fc->type) {
            case VRNA_FC_TYPE_SINGLE:
              c0  += P->internal_loop[j - q - 1];
              break;

            case VRNA_FC_TYPE_COMPARATIVE:
              for (s = 0; s < n_seq; s++) {
                u1  = a2s[s][j - 1] - a2s[s][q];
                c0 += P->internal_loop[u1];
              }
              break;
          }

          if (en == c0)
            return vrna_bt_gquad_mfe(fc, p, q, bp_stack);
        }
      }
    }
  }

  for (p = i + 2;
       p < j - VRNA_GQUAD_MIN_BOX_SIZE;
       p++) {
    l1 = p - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S1[p] != 3)
      continue;

    minl  = j - i + p - MAXLOOP - 2;
    c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minl  = MAX2(c0, minl);
    c0    = j - 1;
    maxl  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxl  = MIN2(c0, maxl);
    for (q = minl; q < maxl; q++) {
      if (S1[q] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0  = energy + e_gq;

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            c0 += P->internal_loop[l1 + j - q - 1];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][p - 1] - a2s[s][i];
              u2  = a2s[s][j - 1] - a2s[s][q];
              c0 += P->internal_loop[u1 + u2];
            }
            break;
        }

        if (en == c0)
          return vrna_bt_gquad_mfe(fc, p, q, bp_stack);
      }
    }
  }

  q = j - 1;
  if (S1[q] == 3)
    for (p = i + 4;
         p < j - VRNA_GQUAD_MIN_BOX_SIZE;
         p++) {
      l1 = p - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S1[p] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0  = energy + e_gq;

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            c0  += P->internal_loop[l1];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][p - 1] - a2s[s][i];
              c0 += P->internal_loop[u1];
            }
            break;
        }

        if (en == c0)
          return vrna_bt_gquad_mfe(fc, p, q, bp_stack);
      }
    }

  return 0;
}


PUBLIC int
vrna_BT_gquad_int(vrna_fold_compound_t  *fc,
                  int                   i,
                  int                   j,
                  int                   en,
                  vrna_bp_stack_t       *bp_stack,
                  unsigned int          *stack_count)
{
  int r = 0;

  if ((fc) &&
      (bp_stack) &&
      (stack_count)) {
    vrna_bps_t bps = vrna_bps_init(4);
    r = vrna_bt_gquad_int(fc, i, j, en, bps);
    while (vrna_bps_size(bps) > 0) {
      vrna_bp_t bp = vrna_bps_pop(bps);
      bp_stack[++(*stack_count)].i = bp.i;
      bp_stack[*stack_count].j = bp.j;
    }
    
    vrna_bps_free(bps);
  }

  return r;
}


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
                          vrna_param_t  *P)
{
  int   energy, dangles, k, l, maxl, minl, c0, l1;
  short si, sj;

  dangles = P->model_details.dangles;
  si      = S[i + 1];
  sj      = S[j - 1];
  energy  = 0;

  if (dangles == 2)
    energy += P->mismatchI[type][si][sj];

  if (type > 2)
    energy += P->TerminalAU;

  k = i + 1;
  if (S[k] == 3) {
    if (k < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minl  = j - i + k - MAXLOOP - 2;
      c0    = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minl  = MAX2(c0, minl);
      c0    = j - 3;
      maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxl  = MIN2(c0, maxl);
      for (l = minl; l < maxl; l++) {
        if (S[l] != 3)
          continue;

        if (c == energy + ggg[k][l - k] + P->internal_loop[j - l - 1]) {
          *p  = k;
          *q  = l;
          return 1;
        }
      }
    }
  }

  for (k = i + 2;
       k < j - VRNA_GQUAD_MIN_BOX_SIZE;
       k++) {
    l1 = k - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S[k] != 3)
      continue;

    minl  = j - i + k - MAXLOOP - 2;
    c0    = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minl  = MAX2(c0, minl);
    c0    = j - 1;
    maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxl  = MIN2(c0, maxl);
    for (l = minl; l < maxl; l++) {
      if (S[l] != 3)
        continue;

      if (c == energy + ggg[k][l - k] + P->internal_loop[l1 + j - l - 1]) {
        *p  = k;
        *q  = l;
        return 1;
      }
    }
  }

  l = j - 1;
  if (S[l] == 3)
    for (k = i + 4;
         k < j - VRNA_GQUAD_MIN_BOX_SIZE;
         k++) {
      l1 = k - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S[k] != 3)
        continue;

      if (c == energy + ggg[k][l - k] + P->internal_loop[l1]) {
        *p  = k;
        *q  = l;
        return 1;
      }
    }

  return 0;
}


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
                                      vrna_param_t  *P)
{
  /*
   * The case that is handled here actually resembles something like
   * an interior loop where the enclosing base pair is of regular
   * kind and the enclosed pair is not a canonical one but a g-quadruplex
   * that should then be decomposed further...
   */
  int mm, dangle_model, k, l, maxl, minl, c0, l1, ss, tt, eee, u1, u2;

  dangle_model = P->model_details.dangles;

  mm = 0;
  for (ss = 0; ss < n_seq; ss++) {
    tt = type[ss];

    if (dangle_model == 2)
      mm += P->mismatchI[tt][S3[ss][i]][S5[ss][j]];

    if (tt > 2)
      mm += P->TerminalAU;
  }

  for (k = i + 2;
       k < j - VRNA_GQUAD_MIN_BOX_SIZE;
       k++) {
    if (S_cons[k] != 3)
      continue;

    l1 = k - i - 1;
    if (l1 > MAXLOOP)
      break;

    minl  = j - i + k - MAXLOOP - 2;
    c0    = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minl  = MAX2(c0, minl);
    c0    = j - 1;
    maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxl  = MIN2(c0, maxl);
    for (l = minl; l < maxl; l++) {
      if (S_cons[l] != 3)
        continue;

      eee = 0;

      for (ss = 0; ss < n_seq; ss++) {
        u1  = a2s[ss][k - 1] - a2s[ss][i];
        u2  = a2s[ss][j - 1] - a2s[ss][l];
        eee += P->internal_loop[u1 + u2];
      }

      c0 = mm +
           ggg[k][l - k] +
           eee;

      if (c == c0) {
        *p  = k;
        *q  = l;
        return 1;
      }
    }
  }
  k = i + 1;
  if (S_cons[k] == 3) {
    if (k < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minl  = j - i + k - MAXLOOP - 2;
      c0    = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minl  = MAX2(c0, minl);
      c0    = j - 3;
      maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxl  = MIN2(c0, maxl);
      for (l = minl; l < maxl; l++) {
        if (S_cons[l] != 3)
          continue;

        eee = 0;

        for (ss = 0; ss < n_seq; ss++) {
          u1  = a2s[ss][j - 1] - a2s[ss][l];
          eee += P->internal_loop[u1];
        }

        if (c == mm + ggg[k][l - k] + eee) {
          *p  = k;
          *q  = l;
          return 1;
        }
      }
    }
  }

  l = j - 1;
  if (S_cons[l] == 3) {
    for (k = i + 4; k < j - VRNA_GQUAD_MIN_BOX_SIZE; k++) {
      l1 = k - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S_cons[k] != 3)
        continue;

      eee = 0;

      for (ss = 0; ss < n_seq; ss++) {
        u1  = a2s[ss][k - 1] - a2s[ss][i];
        eee += P->internal_loop[u1];
      }

      if (c == mm + ggg[k][l - k] + eee) {
        *p  = k;
        *q  = l;
        return 1;
      }
    }
  }

  return 0;
}


PUBLIC
int
vrna_E_gq_intLoop(vrna_fold_compound_t *fc,
                  int           i,
                  int           j)
{
  unsigned int      type, s, n_seq, **a2s;
  int               energy, ge, e_gq, dangles, p, q, l1, minq, maxq, c0, u1, u2;
  short             *S, *S1, si, sj, **SS, **S5, **S3;
  vrna_param_t      *P;
  vrna_md_t         *md;
  vrna_smx_csr(int) *c_gq;

  n_seq   = fc->n_seq;
  S       = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;
  S1      = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding : fc->S_cons;
  SS      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  S5      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S5;
  S3      = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S3;
  a2s     = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->a2s;
  c_gq    = fc->matrices->c_gq;
  P       = fc->params;
  md      = &(P->model_details);
  dangles = md->dangles;
  si      = S1[i + 1];
  sj      = S1[j - 1];
  energy  = 0;

  switch (fc->type) {
    case VRNA_FC_TYPE_SINGLE:
      type    = vrna_get_ptype_md(S[i], S[j], md);
      if (dangles == 2)
        energy += P->mismatchI[type][si][sj];

      if (type > 2)
        energy += P->TerminalAU;
      break;

    case VRNA_FC_TYPE_COMPARATIVE:
      for (s = 0; s < n_seq; s++) {
        type = vrna_get_ptype_md(SS[s][i], SS[s][j], md);
        if (md->dangles == 2)
          energy += P->mismatchI[type][S3[s][i]][S5[s][j]];

        if (type > 2)
          energy += P->TerminalAU;
      }
      break;

    default:
      return INF;
  }

  ge = INF;

  p = i + 1;
  if (S1[p] == 3) {
    if (p < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minq  = j - i + p - MAXLOOP - 2;
      c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minq  = MAX2(c0, minq);
      c0    = j - 3;
      maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxq  = MIN2(c0, maxq);
      for (q = minq; q < maxq; q++) {
        if (S1[q] != 3)
          continue;

#ifndef VRNA_DISABLE_C11_FEATURES
        e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
        e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

        if (e_gq != INF) {
          c0 = energy + e_gq;

          switch (fc->type) {
            case VRNA_FC_TYPE_SINGLE:
              c0  += P->internal_loop[j - q - 1];
              break;

            case VRNA_FC_TYPE_COMPARATIVE:
              for (s = 0; s < n_seq; s++) {
                u1  = a2s[s][j - 1] - a2s[s][q];
                c0 += P->internal_loop[u1];
              }

              break;
          }

          ge  = MIN2(ge, c0);
        }
      }
    }
  }

  for (p = i + 2;
       p < j - VRNA_GQUAD_MIN_BOX_SIZE;
       p++) {
    l1 = p - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S1[p] != 3)
      continue;

    minq  = j - i + p - MAXLOOP - 2;
    c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minq  = MAX2(c0, minq);
    c0    = j - 1;
    maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxq  = MIN2(c0, maxq);
    for (q = minq; q < maxq; q++) {
      if (S1[q] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0  = energy + e_gq;

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            c0 += P->internal_loop[l1 + j - q - 1];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][p - 1] - a2s[s][i];
              u2  = a2s[s][j - 1] - a2s[s][q];
              c0 += P->internal_loop[u1 + u2];
            }
            break;
        }

        ge  = MIN2(ge, c0);
      }
    }
  }

  q = j - 1;
  if (S1[q] == 3)
    for (p = i + 4;
         p < j - VRNA_GQUAD_MIN_BOX_SIZE;
         p++) {
      l1 = p - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S1[p] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0  = energy + e_gq;

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            c0  += P->internal_loop[l1];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][p - 1] - a2s[s][i];
              c0 += P->internal_loop[u1];
            }
            break;
        }

        ge  = MIN2(ge, c0);
      }
    }

  return ge;
}


PUBLIC
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
                              vrna_param_t  *P)
{
  unsigned int  type;
  int           eee, energy, ge, p, q, l1, u1, u2, minq, maxq, c0, s;
  vrna_md_t     *md;

  md      = &(P->model_details);
  energy  = 0;

  for (s = 0; s < n_seq; s++) {
    type = tt[s];
    if (md->dangles == 2)
      energy += P->mismatchI[type][S3[s][i]][S5[s][j]];

    if (type > 2)
      energy += P->TerminalAU;
  }

  ge = INF;

  p = i + 1;
  if (S_cons[p] == 3) {
    if (p < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minq  = j - i + p - MAXLOOP - 2;
      c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minq  = MAX2(c0, minq);
      c0    = j - 3;
      maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxq  = MIN2(c0, maxq);
      for (q = minq; q < maxq; q++) {
        if (S_cons[q] != 3)
          continue;

        eee = 0;

        for (s = 0; s < n_seq; s++) {
          u1  = a2s[s][j - 1] - a2s[s][q];
          eee += P->internal_loop[u1];
        }

        c0 = energy +
             ggg[p][q - p] +
             eee;
        ge = MIN2(ge, c0);
      }
    }
  }

  for (p = i + 2;
       p < j - VRNA_GQUAD_MIN_BOX_SIZE;
       p++) {
    l1 = p - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S_cons[p] != 3)
      continue;

    minq  = j - i + p - MAXLOOP - 2;
    c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minq  = MAX2(c0, minq);
    c0    = j - 1;
    maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxq  = MIN2(c0, maxq);
    for (q = minq; q < maxq; q++) {
      if (S_cons[q] != 3)
        continue;

      eee = 0;

      for (s = 0; s < n_seq; s++) {
        u1  = a2s[s][p - 1] - a2s[s][i];
        u2  = a2s[s][j - 1] - a2s[s][q];
        eee += P->internal_loop[u1 + u2];
      }

      c0 = energy +
           ggg[p][q - p] +
           eee;
      ge = MIN2(ge, c0);
    }
  }

  q = j - 1;
  if (S_cons[q] == 3)
    for (p = i + 4;
         p < j - VRNA_GQUAD_MIN_BOX_SIZE;
         p++) {
      l1 = p - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S_cons[p] != 3)
        continue;

      eee = 0;

      for (s = 0; s < n_seq; s++) {
        u1  = a2s[s][p - 1] - a2s[s][i];
        eee += P->internal_loop[u1];
      }

      c0 = energy +
           ggg[p][q - p] +
           eee;
      ge = MIN2(ge, c0);
    }

  return ge;
}


int *
vrna_E_gq_intLoop_exhaustive(vrna_fold_compound_t *fc,
                            int          i,
                            int          j,
                           int          **p_p,
                           int          **q_p,
                           int          threshold)
{
  int   type;
  int   energy, *ge, e_gq, dangles, p, q, l1, minq, maxq, c0;
  short *S, *S1, si, sj;
  int   cnt = 0;

  vrna_param_t  *P = fc->params;
  vrna_md_t     *md = &(P->model_details);

  vrna_smx_csr(int) *c_gq = fc->matrices->c_gq;

  S       = fc->sequence_encoding2;
  S1       = fc->sequence_encoding;
  type    = vrna_get_ptype_md(S[i], S[j], md);
  dangles = md->dangles;
  si      = S[i + 1];
  sj      = S[j - 1];
  energy  = 0;

  if (dangles == 2)
    energy += P->mismatchI[type][si][sj];

  if (type > 2)
    energy += P->TerminalAU;

  /* guess how many gquads are possible in interval [i+1,j-1] */
  *p_p  = (int *)vrna_alloc(sizeof(int) * 256);
  *q_p  = (int *)vrna_alloc(sizeof(int) * 256);
  ge    = (int *)vrna_alloc(sizeof(int) * 256);

  p = i + 1;
  if (S[p] == 3) {
    if (p < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minq  = j - i + p - MAXLOOP - 2;
      c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minq  = MAX2(c0, minq);
      c0    = j - 3;
      maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxq  = MIN2(c0, maxq);
      for (q = minq; q < maxq; q++) {
        if (S[q] != 3)
          continue;

#ifndef VRNA_DISABLE_C11_FEATURES
        e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
        e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

        if (e_gq != INF) {
          c0 = energy + e_gq + P->internal_loop[j - q - 1];

          if (c0 <= threshold) {
            ge[cnt]       = energy + P->internal_loop[j - q - 1];
            (*p_p)[cnt]   = p;
            (*q_p)[cnt++] = q;
          }
        }
      }
    }
  }

  for (p = i + 2;
       p < j - VRNA_GQUAD_MIN_BOX_SIZE;
       p++) {
    l1 = p - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S[p] != 3)
      continue;

    minq  = j - i + p - MAXLOOP - 2;
    c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minq  = MAX2(c0, minq);
    c0    = j - 1;
    maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxq  = MIN2(c0, maxq);
    for (q = minq; q < maxq; q++) {
      if (S[q] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0 = energy + e_gq + P->internal_loop[l1 + j - q - 1];

        if (c0 <= threshold) {
          ge[cnt]       = energy + P->internal_loop[l1 + j - q - 1];
          (*p_p)[cnt]   = p;
          (*q_p)[cnt++] = q;
        }
      }
    }
  }

  q = j - 1;
  if (S[q] == 3)
    for (p = i + 4;
         p < j - VRNA_GQUAD_MIN_BOX_SIZE;
         p++) {
      l1 = p - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S[p] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      e_gq = vrna_smx_csr_get(c_gq, p, q, INF);
#else
      e_gq = vrna_smx_csr_int_get(c_gq, p, q, INF);
#endif

      if (e_gq != INF) {
        c0 = energy + e_gq + P->internal_loop[l1];
        if (c0 <= threshold) {
          ge[cnt]       = energy + P->internal_loop[l1];
          (*p_p)[cnt]   = p;
          (*q_p)[cnt++] = q;
        }
      }
    }

  (*p_p)[cnt] = -1;

  return ge;
}


PUBLIC
int
E_GQuad_IntLoop_L(int           i,
                  int           j,
                  int           type,
                  short         *S,
                  int           **ggg,
                  int           maxdist,
                  vrna_param_t  *P)
{
  int   energy, ge, dangles, p, q, l1, minq, maxq, c0;
  short si, sj;

  dangles = P->model_details.dangles;
  si      = S[i + 1];
  sj      = S[j - 1];
  energy  = 0;

  if (dangles == 2)
    energy += P->mismatchI[type][si][sj];

  if (type > 2)
    energy += P->TerminalAU;

  ge = INF;

  p = i + 1;
  if (S[p] == 3) {
    if (p < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minq  = j - i + p - MAXLOOP - 2;
      c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minq  = MAX2(c0, minq);
      c0    = j - 3;
      maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxq  = MIN2(c0, maxq);
      for (q = minq; q < maxq; q++) {
        if (S[q] != 3)
          continue;

        c0  = energy + ggg[p][q - p] + P->internal_loop[j - q - 1];
        ge  = MIN2(ge, c0);
      }
    }
  }

  for (p = i + 2;
       p < j - VRNA_GQUAD_MIN_BOX_SIZE;
       p++) {
    l1 = p - i - 1;
    if (l1 > MAXLOOP)
      break;

    if (S[p] != 3)
      continue;

    minq  = j - i + p - MAXLOOP - 2;
    c0    = p + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minq  = MAX2(c0, minq);
    c0    = j - 1;
    maxq  = p + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    maxq  = MIN2(c0, maxq);
    for (q = minq; q < maxq; q++) {
      if (S[q] != 3)
        continue;

      c0  = energy + ggg[p][q - p] + P->internal_loop[l1 + j - q - 1];
      ge  = MIN2(ge, c0);
    }
  }

  q = j - 1;
  if (S[q] == 3)
    for (p = i + 4;
         p < j - VRNA_GQUAD_MIN_BOX_SIZE;
         p++) {
      l1 = p - i - 1;
      if (l1 > MAXLOOP)
        break;

      if (S[p] != 3)
        continue;

      c0  = energy + ggg[p][q - p] + P->internal_loop[l1];
      ge  = MIN2(ge, c0);
    }

  return ge;
}


PUBLIC
FLT_OR_DBL
vrna_exp_E_gq_intLoop(vrna_fold_compound_t *fc,
                      int               i,
                      int               j)
{
  unsigned int      type, s, n_seq, **a2s;
  int               k, l, minl, maxl, u, u1, u2, r;
  FLT_OR_DBL        q, qe, q_g;
  double            *expintern;
  short             *S, *S1, si, sj, **SS, **S5, **S3;
  FLT_OR_DBL        *scale;
  vrna_exp_param_t  *pf_params;
  vrna_md_t         *md;
  vrna_smx_csr(FLT_OR_DBL) *q_gq;

  n_seq     = fc->n_seq;                                                               
  S         = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;       
  S1        = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding : fc->S_cons;  
  SS        = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;                        
  S5        = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S5;                       
  S3        = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S3;                       
  a2s       = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->a2s;                      
  q_gq      = fc->exp_matrices->q_gq;                                                  
  scale     = fc->exp_matrices->scale;
  pf_params = fc->exp_params;
  md        = &(pf_params->model_details);
  dangles   = md->dangles;
  si        = S1[i + 1];
  sj        = S1[j - 1];

  qe = 1.;

  switch (fc->type) {
    case VRNA_FC_TYPE_SINGLE:
      type    = vrna_get_ptype_md(S[i], S[j], md);
      if (dangles == 2)
        qe *=  (FLT_OR_DBL)pf_params->expmismatchI[type][si][sj];

      if (type > 2)
        qe *= (FLT_OR_DBL)pf_params->expTermAU;
      break;

    case VRNA_FC_TYPE_COMPARATIVE:
      for (s = 0; s < n_seq; s++) {
        type = vrna_get_ptype_md(SS[s][i], SS[s][j], md);
        if (md->dangles == 2)
          qe *= (FLT_OR_DBL)pf_params->expmismatchI[type][S3[s][i]][S5[s][j]];

        if (type > 2)
          qe *= (FLT_OR_DBL)pf_params->expTermAU;
      }
      break;

    default:
      return 0.;
  }

  expintern = &(pf_params->expinternal[0]);
  q         = 0;
  k         = i + 1;

  if (S1[k] == 3) {
    if (k < j - VRNA_GQUAD_MIN_BOX_SIZE) {
      minl  = j - MAXLOOP - 1;
      u     = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
      minl  = MAX2(u, minl);
      u     = j - 3;
      maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
      maxl  = MIN2(u, maxl);
      for (l = minl; l < maxl; l++) {
        if (S1[l] != 3)
          continue;

#ifndef VRNA_DISABLE_C11_FEATURES
        q_g = vrna_smx_csr_get(q_gq, k, l, 0.);
#else
        q_g = vrna_smx_csr_FLT_OR_DBL_get(q_gq, k, l, 0.);
#endif

        if (q_g != 0.) {
          q_g *= qe * scale[j - l + 1];

          switch (fc->type) {
            case VRNA_FC_TYPE_SINGLE:
              q_g *= (FLT_OR_DBL)expintern[j - l - 1];
              break;

            case VRNA_FC_TYPE_COMPARATIVE:
              for (s = 0; s < n_seq; s++) {
                u1  = a2s[s][j - 1] - a2s[s][l];
                q_g *= (FLT_OR_DBL)expintern[u1];
              }

              break;
          }

          q += q_g;
        }

      }
    }
  }

  for (k = i + 2;
       k <= j - VRNA_GQUAD_MIN_BOX_SIZE;
       k++) {
    u = k - i - 1;
    if (u > MAXLOOP)
      break;

    if (S1[k] != 3)
      continue;

    minl  = j - i + k - MAXLOOP - 2;
    r     = k + VRNA_GQUAD_MIN_BOX_SIZE - 1;
    minl  = MAX2(r, minl);
    maxl  = k + VRNA_GQUAD_MAX_BOX_SIZE + 1;
    r     = j - 1;
    maxl  = MIN2(r, maxl);
    for (l = minl; l < maxl; l++) {
      if (S1[l] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      q_g = vrna_smx_csr_get(q_gq, k, l, 0.);
#else
      q_g = vrna_smx_csr_FLT_OR_DBL_get(q_gq, k, l, 0.);
#endif

      if (q_g != 0.) {
        q_g *= qe * scale[u + j - l + 1];

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            q_g *= (FLT_OR_DBL)expintern[u + j - l - 1];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][k - 1] - a2s[s][i];
              u2  = a2s[s][j - 1] - a2s[s][l];
              q_g *= (FLT_OR_DBL)expintern[u1 + u2];
            }
            break;
        }

        q += q_g;
      }
    }
  }

  l = j - 1;
  if (S1[l] == 3)
    for (k = i + 4; k <= j - VRNA_GQUAD_MIN_BOX_SIZE; k++) {
      u = k - i - 1;
      if (u > MAXLOOP)
        break;

      if (S1[k] != 3)
        continue;

#ifndef VRNA_DISABLE_C11_FEATURES
      q_g = vrna_smx_csr_get(q_gq, k, l, 0.);
#else
      q_g = vrna_smx_csr_FLT_OR_DBL_get(q_gq, k, l, 0.);
#endif

      if (q_g != 0.) {
        q_g *= qe * scale[u + 2];

        switch (fc->type) {
          case VRNA_FC_TYPE_SINGLE:
            q_g *= (FLT_OR_DBL)expintern[u];
            break;

          case VRNA_FC_TYPE_COMPARATIVE:
            for (s = 0; s < n_seq; s++) {
              u1  = a2s[s][k - 1] - a2s[s][i];
              q_g *= (FLT_OR_DBL)expintern[u1];
            }
            break;
        }

        q += q_g;
      }
    }

  return q;
}


/********************************
 * Here are the G-quadruplex energy
 * contribution functions
 *********************************/
PUBLIC int
E_gquad(int           L,
        int           l[3],
        vrna_param_t  *P)
{
  int i, c = INF;

  for (i = 0; i < 3; i++) {
    if (l[i] > VRNA_GQUAD_MAX_LINKER_LENGTH)
      return c;

    if (l[i] < VRNA_GQUAD_MIN_LINKER_LENGTH)
      return c;
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    return c;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    return c;

  gquad_mfe(0, L, l,
            (void *)(&c),
            (void *)P,
            NULL,
            NULL);
  return c;
}


PUBLIC FLT_OR_DBL
exp_E_gquad(int               L,
            int               l[3],
            vrna_exp_param_t  *pf)
{
  int         i;
  FLT_OR_DBL  q = 0.;

  for (i = 0; i < 3; i++) {
    if (l[i] > VRNA_GQUAD_MAX_LINKER_LENGTH)
      return q;

    if (l[i] < VRNA_GQUAD_MIN_LINKER_LENGTH)
      return q;
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    return q;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    return q;

  gquad_pf(0, L, l,
           (void *)(&q),
           (void *)pf,
           NULL,
           NULL);
  return q;
}


PUBLIC FLT_OR_DBL
exp_E_gquad_ali(int               i,
                int               L,
                int               l[3],
                short             **S,
                unsigned int      **a2s,
                int               n_seq,
                vrna_exp_param_t  *pf)
{
  int         s;
  FLT_OR_DBL  q = 0.;

  for (s = 0; s < 3; s++) {
    if (l[s] > VRNA_GQUAD_MAX_LINKER_LENGTH)
      return q;

    if (l[s] < VRNA_GQUAD_MIN_LINKER_LENGTH)
      return q;
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    return q;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    return q;

  struct gquad_ali_helper gq_help;

  gq_help.S     = S;
  gq_help.a2s   = a2s;
  gq_help.n_seq = n_seq;
  gq_help.pf    = pf;

  gquad_pf_ali(i, L, l,
               (void *)(&q),
               (void *)&gq_help,
               NULL,
               NULL);
  return q;
}


PUBLIC void
E_gquad_ali_en(int          i,
               int          L,
               int          l[3],
               const short  **S,
               unsigned int **a2s,
               unsigned int n_seq,
               vrna_param_t *P,
               int          en[2])
{
  unsigned int  s;
  int           ee, ee2, u1, u2, u3;

  en[0] = en[1] = INF;

  /* check if the quadruplex obeys the canonical form */
  for (s = 0; s < 3; s++) {
    if (l[s] > VRNA_GQUAD_MAX_LINKER_LENGTH)
      return;

    if (l[s] < VRNA_GQUAD_MIN_LINKER_LENGTH)
      return;
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    return;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    return;

  /* compute actual quadruplex contribution for subalignment */
  ee = 0;

  for (s = 0; s < n_seq; s++) {
    u1  = a2s[s][i + L + l[0] - 1] - a2s[s][i + L - 1];
    u2  = a2s[s][i + 2 * L + l[0] + l[1] - 1] - a2s[s][i + 2 * L + l[0] - 1];
    u3  = a2s[s][i + 3 * L + l[0] + l[1] + l[2] - 1] - a2s[s][i + 3 * L + l[0] + l[1] - 1];
    ee  += P->gquad[L][u1 + u2 + u3];
  }

  /* get penalty from incompatible sequences in alignment */
  ee2 = E_gquad_ali_penalty(i, L, l, S, n_seq, P);

  /* assign return values */
  if (ee2 != INF) {
    en[0] = ee;
    en[1] = ee2;
  }
}


PUBLIC vrna_smx_csr(int) *
vrna_gq_pos_mfe(vrna_fold_compound_t *fc)
{
  vrna_smx_csr(int) *gq_mfe_pos = NULL;

  if (fc) {
    int           i, j, n, n2;
    int           *gg;
    vrna_param_t  *P;
    short         *S_enc, *S_tmp;
    struct gquad_ali_helper gq_help;
    void          *data;
    void ( *process_f )(int, int, int *,
                        void *, void *, void *, void *);

    n           = fc->length;
    n2          = 0;
    P           = fc->params;
    S_tmp       = NULL;
    gq_mfe_pos  = vrna_smx_csr_int_init(n + 1);

    switch (fc->type) {
      case VRNA_FC_TYPE_SINGLE:
        S_enc     = fc->sequence_encoding2;
        data      = (void *)P;
        process_f = &gquad_mfe;
        break;

      case VRNA_FC_TYPE_COMPARATIVE:
        S_enc         = fc->S_cons;
        gq_help.S     = fc->S;
        gq_help.a2s   = fc->a2s;
        gq_help.n_seq = fc->n_seq;
        gq_help.P     = P;
        data          = (void *)&gq_help;
        process_f     = &gquad_mfe_ali;
        break;

      default:
        return NULL;
    }

    if (P->model_details.circ) {
      n2 = MIN2(n, VRNA_GQUAD_MAX_BOX_SIZE) - 1;

      S_tmp = (short *)vrna_alloc(sizeof(short) * (n + n2 + 1));
      memcpy(S_tmp, S_enc, sizeof(short) * (n + 1));
      memcpy(S_tmp + (n + 1), S_enc + 1, sizeof(short) * n2);
      S_tmp[0] = n + n2;
      S_enc = S_tmp;
      n += n2;
    }

    gg  = get_g_islands(S_enc);

    FOR_EACH_GQUAD_INC(i, j, 1, n) {
      int e = INF;
      if (i > n - n2)
        break;

      process_gquad_enumeration(gg, i, j,
                                process_f,
                                (void *)(&e),
                                data,
                                NULL,
                                NULL);
      if ((e < INF) && (j - i + 1 <= n - n2)) {
        vrna_log_debug("gquad[%d,%d] = %d l=%d, n= %d, e=%d",
                       i,
                       (j - 1) % (n - n2) + 1,
                       e,
                       j - i + 1,
                       n - n2,
                       e);
#ifndef VRNA_DISABLE_C11_FEATURES
        vrna_smx_csr_insert(gq_mfe_pos, i, (j - 1) % (n - n2) + 1, e);
#else
        vrna_smx_csr_int_insert(gq_mfe_pos, i, (j - 1) % (n - n2) + 1, e);
#endif
      }
    }

    free(S_tmp);
    free(gg);
  }

  return gq_mfe_pos;
}




PUBLIC vrna_smx_csr(FLT_OR_DBL) *
vrna_gq_pos_pf(vrna_fold_compound_t *fc)
{
  vrna_smx_csr(FLT_OR_DBL) *q_gq = NULL;

  if (fc) {
    int                     i, j, n, *gg;
    FLT_OR_DBL              q, *scale;
    vrna_exp_param_t        *pf_params;
    struct gquad_ali_helper gq_help;
    void                    *data;
    void                    ( *process_f )(int, int, int *, void *, void *, void *, void *);

    n           = fc->length;
    pf_params   = fc->exp_params;
    q_gq        = vrna_smx_csr_FLT_OR_DBL_init(n);
    scale       = fc->exp_matrices->scale;

    switch (fc->type) {
      case VRNA_FC_TYPE_SINGLE:
        gg  = get_g_islands(fc->sequence_encoding2);
        data  = (void *)pf_params;
        process_f = &gquad_pf;
        break;

      case VRNA_FC_TYPE_COMPARATIVE:
        gg = get_g_islands(fc->S_cons);
        gq_help.S     = fc->S;
        gq_help.a2s   = fc->a2s;
        gq_help.n_seq = fc->n_seq;
        gq_help.pf    = pf_params;
        data          = (void *)&gq_help;
        process_f     = &gquad_pf_ali;
        break;

      default:
        return NULL;
    }

    FOR_EACH_GQUAD_INC(i, j, 1, n) {
      q = 0.;
      process_gquad_enumeration(gg, i, j,
                                process_f,
                                (void *)(&q),
                                data,
                                NULL,
                                NULL);
      if (q != 0.)
#ifndef VRNA_DISABLE_C11_FEATURES
        vrna_smx_csr_insert(q_gq, i, j, q * scale[j - i + 1]);
#else
        vrna_smx_csr_FLT_OR_DBL_insert(q_gq, i, j, q * scale[j - i + 1]);
#endif
    }

    free(gg);
  }

  return q_gq;
}


PUBLIC int **
get_gquad_L_matrix(short        *S,
                   int          start,
                   int          maxdist,
                   int          n,
                   int          **g,
                   vrna_param_t *P)
{
  return create_L_matrix(S, start, maxdist, n, g, P);
}


PUBLIC void
vrna_gquad_mx_local_update(vrna_fold_compound_t *vc,
                           int                  start)
{
  if (vc->type == VRNA_FC_TYPE_COMPARATIVE) {
    vc->matrices->ggg_local = create_aliL_matrix(
      start,
      vc->window_size,
      vc->length,
      vc->matrices->ggg_local,
      vc->S_cons,
      vc->S,
      vc->a2s,
      vc->n_seq,
      vc->params);
  } else {
    vc->matrices->ggg_local = create_L_matrix(
      vc->sequence_encoding,
      start,
      vc->window_size,
      vc->length,
      vc->matrices->ggg_local,
      vc->params);
  }
}


PRIVATE int **
create_L_matrix(short         *S,
                int           start,
                int           maxdist,
                int           n,
                int           **g,
                vrna_param_t  *P)
{
  int **data;
  int i, j, k, *gg, p, q;

  p   = MAX2(1, start);
  q   = MIN2(n, start + maxdist + 4);
  gg  = get_g_islands_sub(S, p, q);

  if (g) {
    /* we just update the gquadruplex contribution for the current
     * start and rotate the rest */
    data = g;
    /* we re-use the memory allocated previously */
    data[start]               = data[start + maxdist + 5];
    data[start + maxdist + 5] = NULL;

    /* prefill with INF */
    for (i = 0; i < maxdist + 5; i++)
      data[start][i] = INF;

    /*  now we compute contributions for all gquads with 5' delimiter at
     *  position 'start'
     */
    FOR_EACH_GQUAD_AT(start, j, start + maxdist + 4){
      process_gquad_enumeration(gg, start, j,
                                &gquad_mfe,
                                (void *)(&(data[start][j - start])),
                                (void *)P,
                                NULL,
                                NULL);
    }
  } else {
    /* create a new matrix from scratch since this is the first
     * call to this function */

    /* allocate memory and prefill with INF */
    data = (int **)vrna_alloc(sizeof(int *) * (n + 1));
    for (k = n; (k > n - maxdist - 5) && (k >= 0); k--) {
      data[k] = (int *)vrna_alloc(sizeof(int) * (maxdist + 5));
      for (i = 0; i < maxdist + 5; i++)
        data[k][i] = INF;
    }

    /* compute all contributions for the gquads in this interval */
    FOR_EACH_GQUAD(i, j, MAX2(1, n - maxdist - 4), n){
      process_gquad_enumeration(gg, i, j,
                                &gquad_mfe,
                                (void *)(&(data[i][j - i])),
                                (void *)P,
                                NULL,
                                NULL);
    }
  }

  gg += p - 1;
  free(gg);
  return data;
}


PRIVATE int **
create_aliL_matrix(int          start,
                   int          maxdist,
                   int          n,
                   int          **g,
                   short        *S_cons,
                   short        **S,
                   unsigned int **a2s,
                   int          n_seq,
                   vrna_param_t *P)
{
  int **data;
  int i, j, k, *gg, p, q;

  p   = MAX2(1, start);
  q   = MIN2(n, start + maxdist + 4);
  gg  = get_g_islands_sub(S_cons, p, q);

  struct gquad_ali_helper gq_help;

  gq_help.S     = S;
  gq_help.a2s   = a2s;
  gq_help.n_seq = n_seq;
  gq_help.P     = P;

  if (g) {
    /* we just update the gquadruplex contribution for the current
     * start and rotate the rest */
    data = g;
    /* we re-use the memory allocated previously */
    data[start]               = data[start + maxdist + 5];
    data[start + maxdist + 5] = NULL;

    /* prefill with INF */
    for (i = 0; i < maxdist + 5; i++)
      data[start][i] = INF;

    /*  now we compute contributions for all gquads with 5' delimiter at
     *  position 'start'
     */
    FOR_EACH_GQUAD_AT(start, j, start + maxdist + 4){
      process_gquad_enumeration(gg, start, j,
                                &gquad_mfe_ali,
                                (void *)(&(data[start][j - start])),
                                (void *)&gq_help,
                                NULL,
                                NULL);
    }
  } else {
    /* create a new matrix from scratch since this is the first
     * call to this function */

    /* allocate memory and prefill with INF */
    data = (int **)vrna_alloc(sizeof(int *) * (n + 1));
    for (k = n; (k > n - maxdist - 5) && (k >= 0); k--) {
      data[k] = (int *)vrna_alloc(sizeof(int) * (maxdist + 5));
      for (i = 0; i < maxdist + 5; i++)
        data[k][i] = INF;
    }

    /* compute all contributions for the gquads in this interval */
    FOR_EACH_GQUAD(i, j, MAX2(1, n - maxdist - 4), n){
      process_gquad_enumeration(gg, i, j,
                                &gquad_mfe_ali,
                                (void *)(&(data[i][j - i])),
                                (void *)&gq_help,
                                NULL,
                                NULL);
    }
  }

  gg += p - 1;
  free(gg);
  return data;
}


PUBLIC plist *
get_plist_gquad_from_db(const char  *structure,
                        float       pr)
{
  int   x, size, actual_size, L, n, ge, ee, gb, l[3];
  plist *pl;

  actual_size = 0;
  ge          = 0;
  n           = 2;
  size        = strlen(structure);
  pl          = (plist *)vrna_alloc(n * size * sizeof(plist));

  while ((ee = parse_gquad(structure + ge, &L, l)) > 0) {
    ge  += ee;
    gb  = ge - L * 4 - l[0] - l[1] - l[2] + 1;

    /* add pseudo-base pair enclosing gquad */
    if (actual_size >= n * size - 5) {
      n   *= 2;
      pl  = (plist *)vrna_realloc(pl, n * size * sizeof(plist));
    }

    pl[actual_size].i       = gb;
    pl[actual_size].j       = ge;
    pl[actual_size].p       = pr;
    pl[actual_size++].type  = VRNA_PLIST_TYPE_GQUAD;

    for (x = 0; x < L; x++) {
      if (actual_size >= n * size - 5) {
        n   *= 2;
        pl  = (plist *)vrna_realloc(pl, n * size * sizeof(plist));
      }

      pl[actual_size].i       = gb + x;
      pl[actual_size].j       = ge + x - L + 1;
      pl[actual_size].p       = pr;
      pl[actual_size++].type  = VRNA_PLIST_TYPE_TRIPLE;

      pl[actual_size].i       = gb + x;
      pl[actual_size].j       = gb + x + l[0] + L;
      pl[actual_size].p       = pr;
      pl[actual_size++].type  = VRNA_PLIST_TYPE_TRIPLE;

      pl[actual_size].i       = gb + x + l[0] + L;
      pl[actual_size].j       = ge + x - 2 * L - l[2] + 1;
      pl[actual_size].p       = pr;
      pl[actual_size++].type  = VRNA_PLIST_TYPE_TRIPLE;

      pl[actual_size].i       = ge + x - 2 * L - l[2] + 1;
      pl[actual_size].j       = ge + x - L + 1;
      pl[actual_size].p       = pr;
      pl[actual_size++].type  = VRNA_PLIST_TYPE_TRIPLE;
    }
  }

  pl[actual_size].i   = pl[actual_size].j = 0;
  pl[actual_size++].p = 0;
  pl                  = (plist *)vrna_realloc(pl, actual_size * sizeof(plist));
  return pl;
}


PUBLIC void
get_gquad_pattern_mfe(short         *S,
                      int           i,
                      int           j,
                      vrna_param_t  *P,
                      int           *L,
                      int           l[3])
{
  int *gg = get_g_islands_sub(S, i, j);
  int c   = INF;

  process_gquad_enumeration(gg, i, j,
                            &gquad_mfe_pos,
                            (void *)(&c),
                            (void *)P,
                            (void *)L,
                            (void *)l);

  gg += i - 1;
  free(gg);
}


PUBLIC void
get_gquad_pattern_mfe_ali(short         **S,
                          unsigned int  **a2s,
                          short         *S_cons,
                          int           n_seq,
                          int           i,
                          int           j,
                          vrna_param_t  *P,
                          int           *L,
                          int           l[3])
{
  int                     mfe, *gg;
  struct gquad_ali_helper gq_help;

  gg  = get_g_islands_sub(S_cons, i, j);
  mfe = INF;

  gq_help.S     = S;
  gq_help.a2s   = a2s;
  gq_help.n_seq = n_seq;
  gq_help.P     = P;

  process_gquad_enumeration(gg, i, j,
                            &gquad_mfe_ali_pos,
                            (void *)(&mfe),
                            (void *)&gq_help,
                            (void *)L,
                            (void *)l);

  gg += i - 1;
  free(gg);
}


PUBLIC void
get_gquad_pattern_exhaustive(short        *S,
                             int          i,
                             int          j,
                             vrna_param_t *P,
                             int          *L,
                             int          *l,
                             int          threshold)
{
  int *gg = get_g_islands_sub(S, i, j);

  process_gquad_enumeration(gg, i, j,
                            &gquad_pos_exhaustive,
                            (void *)(&threshold),
                            (void *)P,
                            (void *)L,
                            (void *)l);

  gg += i - 1;
  free(gg);
}


PUBLIC void
get_gquad_pattern_pf(short            *S,
                     int              i,
                     int              j,
                     vrna_exp_param_t *pf,
                     int              *L,
                     int              l[3])
{
  int         *gg = get_g_islands_sub(S, i, j);
  FLT_OR_DBL  q   = 0.;

  process_gquad_enumeration(gg, i, j,
                            &gquad_pf_pos,
                            (void *)(&q),
                            (void *)pf,
                            (void *)L,
                            (void *)l);

  gg += i - 1;
  free(gg);
}


PUBLIC void
vrna_get_gquad_pattern_pf(vrna_fold_compound_t  *fc,
                          int                   i,
                          int                   j,
                          int                   *L,
                          int                   l[3])
{
  short             *S  = fc->type == VRNA_FC_TYPE_SINGLE ? fc->sequence_encoding2 : fc->S_cons;
  int               *gg = get_g_islands_sub(S, i, j);
  FLT_OR_DBL        q   = 0.;
  vrna_exp_param_t  *pf = fc->exp_params;

  if (fc->type == VRNA_FC_TYPE_SINGLE) {
    process_gquad_enumeration(gg, i, j,
                              &gquad_pf_pos,
                              (void *)(&q),
                              (void *)pf,
                              (void *)L,
                              (void *)l);
  } else {
    struct gquad_ali_helper gq_help;
    gq_help.S     = fc->S;
    gq_help.a2s   = fc->a2s;
    gq_help.n_seq = fc->n_seq;
    gq_help.pf    = pf;
    gq_help.L     = (int)(*L);
    gq_help.l     = (int *)l;
    process_gquad_enumeration(gg, i, j,
                              &gquad_pf_pos_ali,
                              (void *)(&q),
                              (void *)&gq_help,
                              NULL,
                              NULL);
    *L = gq_help.L;
  }

  gg += i - 1;
  free(gg);
}


PUBLIC plist *
get_plist_gquad_from_pr(short             *S,
                        int               gi,
                        int               gj,
                        vrna_smx_csr(FLT_OR_DBL)  *q_gq,
                        FLT_OR_DBL        *probs,
                        FLT_OR_DBL        *scale,
                        vrna_exp_param_t  *pf)
{
  int L, l[3];

  return get_plist_gquad_from_pr_max(S, gi, gj, q_gq, probs, scale, &L, l, pf);
}


PUBLIC vrna_ep_t *
vrna_plist_gquad_from_pr(vrna_fold_compound_t *fc,
                         int               gi,
                         int               gj)
{
  int L, l[3];

  return vrna_plist_gquad_from_pr_max(fc, gi, gj, &L, l);
}


PUBLIC plist *
get_plist_gquad_from_pr_max(short             *S,
                            int               gi,
                            int               gj,
                            vrna_smx_csr(FLT_OR_DBL)  *q_gq,
                            FLT_OR_DBL        *probs,
                            FLT_OR_DBL        *scale,
                            int               *Lmax,
                            int               lmax[3],
                            vrna_exp_param_t  *pf)
{
  int         n, size, *gg, counter, i, j, *my_index;
  FLT_OR_DBL  pp, *tempprobs;
  plist       *pl;

  n         = S[0];
  size      = (n * (n + 1)) / 2 + 2;
  tempprobs = (FLT_OR_DBL *)vrna_alloc(sizeof(FLT_OR_DBL) * size);
  pl        = (plist *)vrna_alloc((S[0] * S[0]) * sizeof(plist));
  gg        = get_g_islands_sub(S, gi, gj);
  counter   = 0;
  my_index  = vrna_idx_row_wise(n);

  process_gquad_enumeration(gg, gi, gj,
                            &gquad_interact,
                            (void *)tempprobs,
                            (void *)pf,
                            (void *)my_index,
                            NULL);

  pp = 0.;
  process_gquad_enumeration(gg, gi, gj,
                            &gquad_pf_pos,
                            (void *)(&pp),
                            (void *)pf,
                            (void *)Lmax,
                            (void *)lmax);

#ifndef VRNA_DISABLE_C11_FEATURES
  pp = probs[my_index[gi] - gj] * scale[gj - gi + 1] / vrna_smx_csr_get(q_gq, gi, gj, 0.);
#else
  pp = probs[my_index[gi] - gj] * scale[gj - gi + 1] / vrna_smx_csr_FLT_OR_DBL_get(q_gq, gi, gj, 0.);
#endif
  for (i = gi; i < gj; i++) {
    for (j = i; j <= gj; j++) {
      if (tempprobs[my_index[i] - j] > 0.) {
        pl[counter].i       = i;
        pl[counter].j       = j;
        pl[counter].p       = pp *
                              tempprobs[my_index[i] - j];
        pl[counter++].type  = VRNA_PLIST_TYPE_TRIPLE;
      }
    }
  }
  pl[counter].i   = pl[counter].j = 0;
  pl[counter++].p = 0.;
  /* shrink memory to actual size needed */
  pl = (plist *)vrna_realloc(pl, counter * sizeof(plist));

  gg += gi - 1;
  free(gg);
  free(my_index);
  free(tempprobs);
  return pl;
}


PUBLIC plist *
vrna_plist_gquad_from_pr_max(vrna_fold_compound_t *fc,
                             int                  gi,
                             int                  gj,
                             int                  *Lmax,
                             int                  lmax[3])
{
  short             *S;
  int               n, size, *gg, counter, i, j, *my_index;
  FLT_OR_DBL        pp, *tempprobs, *probs, *scale;
  plist             *pl;
  vrna_exp_param_t  *pf;
  vrna_smx_csr(FLT_OR_DBL) *q_gq;

  n         = (int)fc->length;
  pf        = fc->exp_params;
  q_gq      = fc->exp_matrices->q_gq;
  probs     = fc->exp_matrices->probs;
  scale     = fc->exp_matrices->scale;
  S         = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : fc->S_cons;
  size      = (n * (n + 1)) / 2 + 2;
  tempprobs = (FLT_OR_DBL *)vrna_alloc(sizeof(FLT_OR_DBL) * size);
  pl        = (plist *)vrna_alloc((n * n) * sizeof(plist));
  gg        = get_g_islands_sub(S, gi, gj);
  counter   = 0;
  my_index  = vrna_idx_row_wise(n);

  pp = 0.;

  if (fc->type == VRNA_FC_TYPE_SINGLE) {
    process_gquad_enumeration(gg, gi, gj,
                              &gquad_interact,
                              (void *)tempprobs,
                              (void *)pf,
                              (void *)my_index,
                              NULL);

    process_gquad_enumeration(gg, gi, gj,
                              &gquad_pf_pos,
                              (void *)(&pp),
                              (void *)pf,
                              (void *)Lmax,
                              (void *)lmax);
  } else {
    struct gquad_ali_helper gq_help;
    gq_help.S     = fc->S;
    gq_help.a2s   = fc->a2s;
    gq_help.n_seq = fc->n_seq;
    gq_help.pf    = pf;
    gq_help.L     = *Lmax;
    gq_help.l     = &(lmax[0]);

    process_gquad_enumeration(gg, gi, gj,
                              &gquad_interact_ali,
                              (void *)tempprobs,
                              (void *)my_index,
                              (void *)&(gq_help),
                              NULL);

    process_gquad_enumeration(gg, gi, gj,
                              &gquad_pf_pos_ali,
                              (void *)(&pp),
                              (void *)&gq_help,
                              NULL,
                              NULL);
    *Lmax = gq_help.L;
  }

  pp = probs[my_index[gi] - gj] *
       scale[gj - gi + 1] /
#ifndef VRNA_DISABLE_C11_FEATURES
       vrna_smx_csr_get(q_gq, gi, gj, 0.);
#else
       vrna_smx_csr_FLT_OR_DBL_get(q_gq, gi, gj, 0.);
#endif

  for (i = gi; i < gj; i++) {
    for (j = i; j <= gj; j++) {
      if (tempprobs[my_index[i] - j] > 0.) {
        pl[counter].i       = i;
        pl[counter].j       = j;
        pl[counter].p       = pp *
                              tempprobs[my_index[i] - j];
        pl[counter++].type  = VRNA_PLIST_TYPE_TRIPLE;
      }
    }
  }
  pl[counter].i   = pl[counter].j = 0;
  pl[counter++].p = 0.;
  /* shrink memory to actual size needed */
  pl = (plist *)vrna_realloc(pl, counter * sizeof(plist));

  gg += gi - 1;
  free(gg);
  free(my_index);
  free(tempprobs);
  return pl;
}


PUBLIC int
get_gquad_count(short *S,
                int   i,
                int   j)
{
  int *gg = get_g_islands_sub(S, i, j);
  int p, q, counter = 0;

  FOR_EACH_GQUAD(p, q, i, j)
  process_gquad_enumeration(gg, p, q,
                            &gquad_count,
                            (void *)(&counter),
                            NULL,
                            NULL,
                            NULL);

  gg += i - 1;
  free(gg);
  return counter;
}


PUBLIC int
get_gquad_layer_count(short *S,
                      int   i,
                      int   j)
{
  int *gg = get_g_islands_sub(S, i, j);
  int p, q, counter = 0;

  FOR_EACH_GQUAD(p, q, i, j)
  process_gquad_enumeration(gg, p, q,
                            &gquad_count_layers,
                            (void *)(&counter),
                            NULL,
                            NULL,
                            NULL);

  gg += i - 1;
  free(gg);
  return counter;
}


PUBLIC int
parse_gquad(const char  *struc,
            int         *L,
            int         l[3])
{
  int i, il, start, end, len;

  for (i = 0; struc[i] && struc[i] != '+'; i++);
  if (struc[i] == '+') {
    /* start of gquad */
    for (il = 0; il <= 3; il++) {
      start = i; /* pos of first '+' */
      while (struc[++i] == '+')
        if ((il) && (i - start == *L))
          break;

      end = i;
      len = end - start;
      if (il == 0)
        *L = len;
      else if (len != *L) {
        vrna_log_error("unequal stack lengths in gquad");
        return 0;
      }
      if (il == 3)
        break;

      while (struc[++i] == '.'); /* linker */
      l[il] = i - end;
      if (struc[i] != '+') {
        vrna_log_error("illegal character in gquad linker region");
        return 0;
      }
    }
  } else {
    return 0;
  }

  /* printf("gquad at %d %d %d %d %d\n", end, *L, l[0], l[1], l[2]); */
  return end;
}


/*
 #########################################
 # BEGIN OF PRIVATE FUNCTION DEFINITIONS #
 #          (internal use only)          #
 #########################################
 */
PRIVATE int
E_gquad_ali_penalty(int           i,
                    int           L,
                    int           l[3],
                    const short   **S,
                    unsigned int  n_seq,
                    vrna_param_t  *P)
{
  unsigned int mm[2];

  count_gquad_layer_mismatches(i, L, l, S, n_seq, mm);

  if (mm[1] > P->gquadLayerMismatchMax)
    return INF;
  else
    return P->gquadLayerMismatch * mm[0];
}


PRIVATE FLT_OR_DBL
exp_E_gquad_ali_penalty(int               i,
                        int               L,
                        int               l[3],
                        const short       **S,
                        unsigned int      n_seq,
                        vrna_exp_param_t  *pf)
{
  unsigned int mm[2];

  count_gquad_layer_mismatches(i, L, l, S, n_seq, mm);

  if (mm[1] > pf->gquadLayerMismatchMax)
    return (FLT_OR_DBL)0.;
  else
    return (FLT_OR_DBL)pow(pf->expgquadLayerMismatch, (double)mm[0]);
}


PRIVATE void
count_gquad_layer_mismatches(int          i,
                             int          L,
                             int          l[3],
                             const short  **S,
                             unsigned int n_seq,
                             unsigned int mm[2])
{
  unsigned int  s;
  int           cnt;

  mm[0] = mm[1] = 0;

  /* check for compatibility in the alignment */
  for (s = 0; s < n_seq; s++) {
    unsigned int  ld        = 0; /* !=0 if layer destruction was detected */
    unsigned int  mismatch  = 0;

    /* check bottom layer */
    if (S[s][i] != 3)
      ld |= 1U;

    if (S[s][i + L + l[0]] != 3)
      ld |= 2U;

    if (S[s][i + 2 * L + l[0] + l[1]] != 3)
      ld |= 4U;

    if (S[s][i + 3 * L + l[0] + l[1] + l[2]] != 3)
      ld |= 8U;

    /* add 1x penalty for missing bottom layer */
    if (ld)
      mismatch++;

    /* check top layer */
    ld = 0;
    if (S[s][i + L - 1] != 3)
      ld |= 1U;

    if (S[s][i + 2 * L + l[0] - 1] != 3)
      ld |= 2U;

    if (S[s][i + 3 * L + l[0] + l[1] - 1] != 3)
      ld |= 4U;

    if (S[s][i + 4 * L + l[0] + l[1] + l[2] - 1] != 3)
      ld |= 8U;

    /* add 1x penalty for missing top layer */
    if (ld)
      mismatch++;

    /* check inner layers */
    ld = 0;
    for (cnt = 1; cnt < L - 1; cnt++) {
      if (S[s][i + cnt] != 3)
        ld |= 1U;

      if (S[s][i + L + l[0] + cnt] != 3)
        ld |= 2U;

      if (S[s][i + 2 * L + l[0] + l[1] + cnt] != 3)
        ld |= 4U;

      if (S[s][i + 3 * L + l[0] + l[1] + l[2] + cnt] != 3)
        ld |= 8U;

      /* add 2x penalty for missing inner layer */
      if (ld)
        mismatch += 2;
    }

    mm[0] += mismatch;

    if (mismatch >= 2 * (L - 1))
      mm[1]++;
  }
}


PRIVATE void
gquad_mfe(int   i,
          int   L,
          int   *l,
          void  *data,
          void  *P,
          void  *NA,
          void  *NA2)
{
  int cc = ((vrna_param_t *)P)->gquad[L][l[0] + l[1] + l[2]];

  if (cc < *((int *)data))
    *((int *)data) = cc;
}


PRIVATE void
gquad_mfe_pos(int   i,
              int   L,
              int   *l,
              void  *data,
              void  *P,
              void  *Lmfe,
              void  *lmfe)
{
  int cc = ((vrna_param_t *)P)->gquad[L][l[0] + l[1] + l[2]];

  if (cc < *((int *)data)) {
    *((int *)data)        = cc;
    *((int *)Lmfe)        = L;
    *((int *)lmfe)        = l[0];
    *(((int *)lmfe) + 1)  = l[1];
    *(((int *)lmfe) + 2)  = l[2];
  }
}


PRIVATE void
gquad_mfe_ali_pos(int   i,
                  int   L,
                  int   *l,
                  void  *data,
                  void  *helper,
                  void  *Lmfe,
                  void  *lmfe)
{
  int cc = INF;

  gquad_mfe_ali(i, L, l, (void *)&cc, helper, NULL, NULL);

  if (cc < *((int *)data)) {
    *((int *)data)        = cc;
    *((int *)Lmfe)        = L;
    *((int *)lmfe)        = l[0];
    *(((int *)lmfe) + 1)  = l[1];
    *(((int *)lmfe) + 2)  = l[2];
  }
}


PRIVATE
void
gquad_pos_exhaustive(int  i,
                     int  L,
                     int  *l,
                     void *data,
                     void *P,
                     void *Lex,
                     void *lex)
{
  int cnt;
  int cc = ((vrna_param_t *)P)->gquad[L][l[0] + l[1] + l[2]];

  if (cc <= *((int *)data)) {
    /*  since Lex is an array of L values and lex an
     *  array of l triples we need to find out where
     *  the current gquad position is to be stored...
     * the below implementation might be slow but we
     * still use it for now
     */
    for (cnt = 0; ((int *)Lex)[cnt] != -1; cnt++);

    *((int *)Lex + cnt)             = L;
    *((int *)Lex + cnt + 1)         = -1;
    *(((int *)lex) + (3 * cnt) + 0) = l[0];
    *(((int *)lex) + (3 * cnt) + 1) = l[1];
    *(((int *)lex) + (3 * cnt) + 2) = l[2];
  }
}


PRIVATE
void
gquad_count(int   i,
            int   L,
            int   *l,
            void  *data,
            void  *NA,
            void  *NA2,
            void  *NA3)
{
  *((int *)data) += 1;
}


PRIVATE
void
gquad_count_layers(int  i,
                   int  L,
                   int  *l,
                   void *data,
                   void *NA,
                   void *NA2,
                   void *NA3)
{
  *((int *)data) += L;
}


PRIVATE void
gquad_pf(int  i,
         int  L,
         int  *l,
         void *data,
         void *pf,
         void *NA,
         void *NA2)
{
  *((FLT_OR_DBL *)data) += ((vrna_exp_param_t *)pf)->expgquad[L][l[0] + l[1] + l[2]];
}


PRIVATE void
gquad_pf_ali(int  i,
             int  L,
             int  *l,
             void *data,
             void *helper,
             void *NA,
             void *NA2)
{
  short                   **S;
  unsigned int            **a2s;
  int                     u1, u2, u3, s, n_seq;
  FLT_OR_DBL              penalty;
  vrna_exp_param_t        *pf;
  struct gquad_ali_helper *gq_help;

  gq_help = (struct gquad_ali_helper *)helper;
  S       = gq_help->S;
  a2s     = gq_help->a2s;
  n_seq   = gq_help->n_seq;
  pf      = gq_help->pf;
  penalty = exp_E_gquad_ali_penalty(i, L, l, (const short **)S, (unsigned int)n_seq, pf);

  if (penalty != 0.) {
    double q = 1.;
    for (s = 0; s < n_seq; s++) {
      u1  = a2s[s][i + L + l[0] - 1] - a2s[s][i + L - 1];
      u2  = a2s[s][i + 2 * L + l[0] + l[1] - 1] - a2s[s][i + 2 * L + l[0] - 1];
      u3  = a2s[s][i + 3 * L + l[0] + l[1] + l[2] - 1] - a2s[s][i + 3 * L + l[0] + l[1] - 1];
      q   *= pf->expgquad[L][u1 + u2 + u3];
    }
    *((FLT_OR_DBL *)data) += q * penalty;
  }
}


PRIVATE void
gquad_pf_pos(int  i,
             int  L,
             int  *l,
             void *data,
             void *pf,
             void *Lmax,
             void *lmax)
{
  FLT_OR_DBL gq = 0.;

  gquad_pf(i, L, l, (void *)&gq, pf, NULL, NULL);

  if (gq > *((FLT_OR_DBL *)data)) {
    *((FLT_OR_DBL *)data) = gq;
    *((int *)Lmax)        = L;
    *((int *)lmax)        = l[0];
    *(((int *)lmax) + 1)  = l[1];
    *(((int *)lmax) + 2)  = l[2];
  }
}


PRIVATE void
gquad_pf_pos_ali(int  i,
                 int  L,
                 int  *l,
                 void *data,
                 void *helper,
                 void *NA,
                 void *NA2)
{
  FLT_OR_DBL              gq        = 0.;
  struct gquad_ali_helper *gq_help  = (struct gquad_ali_helper *)helper;

  gquad_pf_ali(i, L, l, (void *)&gq, helper, NULL, NULL);

  if (gq > *((FLT_OR_DBL *)data)) {
    *((FLT_OR_DBL *)data) = gq;
    gq_help->L            = L;
    gq_help->l[0]         = l[0];
    gq_help->l[1]         = l[1];
    gq_help->l[2]         = l[2];
  }
}


PRIVATE void
gquad_mfe_ali(int   i,
              int   L,
              int   *l,
              void  *data,
              void  *helper,
              void  *NA,
              void  *NA2)
{
  int j, en[2], cc;

  en[0] = en[1] = INF;

  for (j = 0; j < 3; j++) {
    if (l[j] > VRNA_GQUAD_MAX_LINKER_LENGTH)
      return;

    if (l[j] < VRNA_GQUAD_MIN_LINKER_LENGTH)
      return;
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    return;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    return;

  gquad_mfe_ali_en(i, L, l, (void *)(&(en[0])), helper, NULL, NULL);
  if (en[1] != INF) {
    cc = en[0] + en[1];
    if (cc < *((int *)data))
      *((int *)data) = cc;
  }
}


PRIVATE void
gquad_mfe_ali_en(int  i,
                 int  L,
                 int  *l,
                 void *data,
                 void *helper,
                 void *NA,
                 void *NA2)
{
  short                   **S;
  unsigned int            **a2s;
  int                     en[2], cc, dd, s, n_seq, u1, u2, u3;
  vrna_param_t            *P;
  struct gquad_ali_helper *gq_help;

  gq_help = (struct gquad_ali_helper *)helper;
  S       = gq_help->S;
  a2s     = gq_help->a2s;
  n_seq   = gq_help->n_seq;
  P       = gq_help->P;

  for (en[0] = s = 0; s < n_seq; s++) {
    u1    = a2s[s][i + L + l[0] - 1] - a2s[s][i + L - 1];
    u2    = a2s[s][i + 2 * L + l[0] + l[1] - 1] - a2s[s][i + 2 * L + l[0] - 1];
    u3    = a2s[s][i + 3 * L + l[0] + l[1] + l[2] - 1] - a2s[s][i + 3 * L + l[0] + l[1] - 1];
    en[0] += P->gquad[L][u1 + u2 + u3];
  }
  en[1] = E_gquad_ali_penalty(i, L, l, (const short **)S, (unsigned int)n_seq, P);

  if (en[1] != INF) {
    cc  = en[0] + en[1];
    dd  = ((int *)data)[0] + ((int *)data)[1];
    if (cc < dd) {
      ((int *)data)[0]  = en[0];
      ((int *)data)[1]  = en[1];
    }
  }
}


PRIVATE void
gquad_interact(int  i,
               int  L,
               int  *l,
               void *data,
               void *pf,
               void *index,
               void *NA2)
{
  int         x, *idx;
  FLT_OR_DBL  gq, *pp;

  idx = (int *)index;
  pp  = (FLT_OR_DBL *)data;
  gq  = exp_E_gquad(L, l, (vrna_exp_param_t *)pf);

  for (x = 0; x < L; x++) {
    pp[idx[i + x] - (i + x + 3 * L + l[0] + l[1] + l[2])]                       += gq;
    pp[idx[i + x] - (i + x + L + l[0])]                                         += gq;
    pp[idx[i + x + L + l[0]] - (i + x + 2 * L + l[0] + l[1])]                   += gq;
    pp[idx[i + x + 2 * L + l[0] + l[1]] - (i + x + 3 * L + l[0] + l[1] + l[2])] += gq;
  }
}


PRIVATE void
gquad_interact_ali(int  i,
                   int  L,
                   int  *l,
                   void *data,
                   void *index,
                   void *helper,
                   void *NA)
{
  int         x, *idx, bad;
  FLT_OR_DBL  gq, *pp;

  idx = (int *)index;
  pp  = (FLT_OR_DBL *)data;
  bad = 0;

  for (x = 0; x < 3; x++) {
    if (l[x] > VRNA_GQUAD_MAX_LINKER_LENGTH) {
      bad = 1;
      break;
    }

    if (l[x] < VRNA_GQUAD_MIN_LINKER_LENGTH) {
      bad = 1;
      break;
    }
  }
  if (L > VRNA_GQUAD_MAX_STACK_SIZE)
    bad = 1;

  if (L < VRNA_GQUAD_MIN_STACK_SIZE)
    bad = 1;

  gq = 0.;

  if (!bad) {
    gquad_pf_ali(i, L, l,
                 (void *)(&gq),
                 helper,
                 NULL,
                 NULL);
  }

  for (x = 0; x < L; x++) {
    pp[idx[i + x] - (i + x + 3 * L + l[0] + l[1] + l[2])]                       += gq;
    pp[idx[i + x] - (i + x + L + l[0])]                                         += gq;
    pp[idx[i + x + L + l[0]] - (i + x + 2 * L + l[0] + l[1])]                   += gq;
    pp[idx[i + x + 2 * L + l[0] + l[1]] - (i + x + 3 * L + l[0] + l[1] + l[2])] += gq;
  }
}


PRIVATE INLINE int *
get_g_islands(short *S)
{
  return get_g_islands_sub(S, 1, S[0]);
}


PRIVATE INLINE int *
get_g_islands_sub(short *S,
                  int   i,
                  int   j)
{
  int x, *gg;

  gg  = (int *)vrna_alloc(sizeof(int) * (j - i + 2));
  gg  -= i - 1;

  if (S[j] == 3)
    gg[j] = 1;

  for (x = j - 1; x >= i; x--)
    if (S[x] == 3)
      gg[x] = gg[x + 1] + 1;

  return gg;
}


/**
 *  We could've also created a macro that loops over all G-quadruplexes
 *  delimited by i and j. However, for the fun of it we use this function
 *  that receives a pointer to a callback function which in turn does the
 *  actual computation for each quadruplex found.
 */
PRIVATE
void
process_gquad_enumeration(int *gg,
                          int i,
                          int j,
                          void ( *f )(int, int, int *,
                                      void *, void *, void *, void *),
                          void *data,
                          void *P,
                          void *aux1,
                          void *aux2)
{
  int L, l[3], n, max_linker, maxl0, maxl1;

  n = j - i + 1;

  if ((n >= VRNA_GQUAD_MIN_BOX_SIZE) && (n <= VRNA_GQUAD_MAX_BOX_SIZE)) {
    for (L = MIN2(gg[i], VRNA_GQUAD_MAX_STACK_SIZE);
         L >= VRNA_GQUAD_MIN_STACK_SIZE;
         L--)
      if (gg[j - L + 1] >= L) {
        max_linker = n - 4 * L;
        if ((max_linker >= 3 * VRNA_GQUAD_MIN_LINKER_LENGTH)
            && (max_linker <= 3 * VRNA_GQUAD_MAX_LINKER_LENGTH)) {
          maxl0 = MIN2(VRNA_GQUAD_MAX_LINKER_LENGTH,
                       max_linker - 2 * VRNA_GQUAD_MIN_LINKER_LENGTH
                       );
          for (l[0] = VRNA_GQUAD_MIN_LINKER_LENGTH;
               l[0] <= maxl0;
               l[0]++)
            if (gg[i + L + l[0]] >= L) {
              maxl1 = MIN2(VRNA_GQUAD_MAX_LINKER_LENGTH,
                           max_linker - l[0] - VRNA_GQUAD_MIN_LINKER_LENGTH
                           );
              for (l[1] = VRNA_GQUAD_MIN_LINKER_LENGTH;
                   l[1] <= maxl1;
                   l[1]++)
                if (gg[i + 2 * L + l[0] + l[1]] >= L) {
                  l[2] = max_linker - l[0] - l[1];
                  f(i, L, &(l[0]), data, P, aux1, aux2);
                }
            }
        }
      }
  }
}

#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY
/********************************
 * Now, the triangular matrix
 * generators for the G-quadruplex
 * contributions are following
 *********************************/
PUBLIC int *
get_gquad_matrix(short        *S,
                 vrna_param_t *P)
{
  int n, size, i, j, *gg, *my_index, *data;

  n         = S[0];
  my_index  = vrna_idx_col_wise(n);
  gg        = get_g_islands(S);
  size      = (n * (n + 1)) / 2 + 2;
  data      = (int *)vrna_alloc(sizeof(int) * size);

  /* prefill the upper triangular matrix with INF */
  for (i = 0; i < size; i++)
    data[i] = INF;

  FOR_EACH_GQUAD(i, j, 1, n){
    process_gquad_enumeration(gg, i, j,
                              &gquad_mfe,
                              (void *)(&(data[my_index[j] + i])),
                              (void *)P,
                              NULL,
                              NULL);
  }

  free(my_index);
  free(gg);
  return data;
}


PUBLIC FLT_OR_DBL *
get_gquad_pf_matrix(short             *S,
                    FLT_OR_DBL        *scale,
                    vrna_exp_param_t  *pf)
{
  int         n, size, *gg, i, j, *my_index;
  FLT_OR_DBL  *data;


  n         = S[0];
  size      = (n * (n + 1)) / 2 + 2;
  data      = (FLT_OR_DBL *)vrna_alloc(sizeof(FLT_OR_DBL) * size);
  gg        = get_g_islands(S);
  my_index  = vrna_idx_row_wise(n);

  FOR_EACH_GQUAD(i, j, 1, n){
    process_gquad_enumeration(gg, i, j,
                              &gquad_pf,
                              (void *)(&(data[my_index[i] - j])),
                              (void *)pf,
                              NULL,
                              NULL);
    data[my_index[i] - j] *= scale[j - i + 1];
  }

  free(my_index);
  free(gg);
  return data;
}

PUBLIC FLT_OR_DBL *
get_gquad_pf_matrix_comparative(unsigned int      n,
                                short             *S_cons,
                                short             **S,
                                unsigned int      **a2s,
                                FLT_OR_DBL        *scale,
                                unsigned int      n_seq,
                                vrna_exp_param_t  *pf)
{
  int                     size, *gg, i, j, *my_index;
  FLT_OR_DBL              *data;
  struct gquad_ali_helper gq_help;


  size          = (n * (n + 1)) / 2 + 2;
  data          = (FLT_OR_DBL *)vrna_alloc(sizeof(FLT_OR_DBL) * size);
  gg            = get_g_islands(S_cons);
  my_index      = vrna_idx_row_wise(n);
  gq_help.S     = S;
  gq_help.a2s   = a2s;
  gq_help.n_seq = n_seq;
  gq_help.pf    = pf;

  FOR_EACH_GQUAD(i, j, 1, n){
    process_gquad_enumeration(gg, i, j,
                              &gquad_pf_ali,
                              (void *)(&(data[my_index[i] - j])),
                              (void *)&gq_help,
                              NULL,
                              NULL);
    data[my_index[i] - j] *= scale[j - i + 1];
  }

  free(my_index);
  free(gg);
  return data;
}


PUBLIC int *
get_gquad_ali_matrix(unsigned int n,
                     short        *S_cons,
                     short        **S,
                     unsigned int **a2s,
                     int          n_seq,
                     vrna_param_t *P)
{
  int                     size, *data, *gg;
  int                     i, j, *my_index;
  struct gquad_ali_helper gq_help;

  size      = (n * (n + 1)) / 2 + 2;
  data      = (int *)vrna_alloc(sizeof(int) * size);
  gg        = get_g_islands(S_cons);
  my_index  = vrna_idx_col_wise(n);

  gq_help.S     = S;
  gq_help.a2s   = a2s;
  gq_help.n_seq = n_seq;
  gq_help.P     = P;

  /* prefill the upper triangular matrix with INF */
  for (i = 0; i < size; i++)
    data[i] = INF;

  FOR_EACH_GQUAD(i, j, 1, n){
    process_gquad_enumeration(gg, i, j,
                              &gquad_mfe_ali,
                              (void *)(&(data[my_index[j] + i])),
                              (void *)&gq_help,
                              NULL,
                              NULL);
  }

  free(my_index);
  free(gg);
  return data;
}


#endif