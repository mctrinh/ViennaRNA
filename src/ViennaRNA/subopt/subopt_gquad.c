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
#include "ViennaRNA/alphabet.h"
#include "ViennaRNA/subopt/gquad.h"

#include "ViennaRNA/loops/gquad_intern.h"

/*
 #################################
 # PRIVATE FUNCTION DECLARATIONS #
 #################################
 */

/*
 #########################################
 # BEGIN OF PUBLIC FUNCTION DEFINITIONS  #
 #      (all available in RNAlib)        #
 #########################################
 */

vrna_array(int)
vrna_gq_int_loop_subopt(vrna_fold_compound_t * fc,
                        unsigned int i,
                        unsigned int j,
                        vrna_array(int) * ps,
                        vrna_array(int) * qs,
                        int threshold){
  unsigned int   type, p, q, l1, minq, maxq;
  int   energy, *ge, e_gq, dangles, c0;
  short *S, *S1, si, sj;

  ge  = NULL;
  *ps = NULL;
  *qs = NULL;

  if ((fc) &&
      (ps) &&
      (qs)) {
    vrna_param_t  *P  = fc->params;
    vrna_md_t     *md = &(P->model_details);

    vrna_smx_csr(int) * c_gq = fc->matrices->c_gq;

    S       = fc->sequence_encoding2;
    S1      = fc->sequence_encoding;
    type    = vrna_get_ptype_md(S[i], S[j], md);
    dangles = md->dangles;
    si      = S1[i + 1];
    sj      = S1[j - 1];
    energy  = 0;

    if (dangles == 2)
      energy += P->mismatchI[type][si][sj];

    if (type > 2)
      energy += P->TerminalAU;

    vrna_array_init(*ps);
    vrna_array_init(*qs);
    vrna_array_init(ge);

    p = i + 1;
    if (S[p] == 3) {
      if (p + VRNA_GQUAD_MIN_BOX_SIZE < j) {
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
              vrna_array_append(ge, energy + P->internal_loop[j - q - 1]);
              vrna_array_append(*ps, p);
              vrna_array_append(*qs, q);
            }
          }
        }
      }
    }

    for (p = i + 2;
         p + VRNA_GQUAD_MIN_BOX_SIZE < j;
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
            vrna_array_append(ge, energy + P->internal_loop[l1 + j - q - 1]);
            vrna_array_append(*ps, p);
            vrna_array_append(*qs, q);
          }
        }
      }
    }

    q = j - 1;
    if (S[q] == 3)
      for (p = i + 4;
           p + VRNA_GQUAD_MIN_BOX_SIZE < j;
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
            vrna_array_append(ge, energy + P->internal_loop[l1]);
            vrna_array_append(*ps, p);
            vrna_array_append(*qs, q);
          }
        }
      }
  }

  return ge;
}


/*
 #########################################
 # BEGIN OF PRIVATE FUNCTION DEFINITIONS #
 #          (internal use only)          #
 #########################################
 */
