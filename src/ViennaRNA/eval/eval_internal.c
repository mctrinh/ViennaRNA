#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/alphabet.h"
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/constraints/hard.h"
#include "ViennaRNA/constraints/soft.h"
#include "ViennaRNA/loops/external.h"
#include "ViennaRNA/loops/gquad.h"
#include "ViennaRNA/structured_domains.h"
#include "ViennaRNA/unstructured_domains.h"
#include "ViennaRNA/eval/structures.h"
#include "ViennaRNA/loops/internal.h"


#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif

#include "ViennaRNA/loops/internal_hc.inc"
#include "ViennaRNA/loops/internal_sc.inc"

/*
 #################################
 # PRIVATE FUNCTION DECLARATIONS #
 #################################
 */

PRIVATE INLINE int
eval_int_loop(vrna_fold_compound_t  *fc,
              unsigned int          i,
              unsigned int          j,
              unsigned int          k,
              unsigned int          l,
              unsigned int          options);


PRIVATE INLINE int
eval_ext_int_loop(vrna_fold_compound_t  *fc,
                  unsigned int          i,
                  unsigned int          j,
                  unsigned int          k,
                  unsigned int          l,
                  unsigned int          options);


/*
 #################################
 # BEGIN OF FUNCTION DEFINITIONS #
 #################################
 */
PUBLIC int
vrna_E_internal(unsigned int  n1,
                unsigned int  n2,
                unsigned int  type,
                unsigned int  type_2,
                int           si1,
                int           sj1,
                int           sp1,
                int           sq1,
                vrna_param_t  *P)
{
  /* compute energy of degree 2 loop (stack bulge or interior) */
  unsigned int  nl, ns, u, backbones, no_close;
  int           energy, salt_stack_correction, salt_loop_correction;

  energy = INF;

  if (P) {
    no_close              = 0;
    salt_stack_correction = P->SaltStack;
    salt_loop_correction  = 0;

    if ((P->model_details.noGUclosure) &&
        ((type_2 == 3) || (type_2 == 4) || (type == 3) || (type == 4)))
      no_close = 1;

    if (n1 > n2) {
      nl  = n1;
      ns  = n2;
    } else {
      nl  = n2;
      ns  = n1;
    }

    if (nl == 0)  /* stack */
      return  P->stack[type][type_2] +
              salt_stack_correction;

    if (no_close)
      return INF;

    if (P->model_details.salt != VRNA_MODEL_DEFAULT_SALT) {
      /* salt correction for loop */
      backbones = nl + ns + 2;
      if (backbones <= MAXLOOP+1)
        salt_loop_correction = P->SaltLoop[backbones];
      else
        salt_loop_correction = vrna_salt_loop_int(backbones,
                                                  P->model_details.salt,
                                                  P->temperature + K0,
                                                  P->model_details.backbone_length);
    }

    energy = 0;

    switch (ns) {
      case 0:
        /* bulge */
        energy = (nl <= MAXLOOP) ?
                  P->bulge[nl] :
                 (P->bulge[30] + (int)(P->lxc * log(nl / 30.)));
        if (nl == 1) { /* add stacking energy for 1-bulges */
          energy += P->stack[type][type_2];
        } else { 
          if (type > 2)
            energy += P->TerminalAU;

          if (type_2 > 2)
            energy += P->TerminalAU;
        }

        break;

      case 1:
        if (nl == 1) { /* 1x1 loop */
          energy = P->int11[type][type_2][si1][sj1];
        } else if (nl == 2) {
          /* 2x1 loop */
          if (n1 == 1)
            energy = P->int21[type][type_2][si1][sq1][sj1];
          else
            energy = P->int21[type_2][type][sq1][si1][sp1];
        } else {
          /* 1xn loop */
          energy = (nl + 1 <= MAXLOOP) ?
                    (P->internal_loop[nl + 1]) :
                    (P->internal_loop[30] + (int)(P->lxc * log((nl + 1) / 30.)));
          energy += MIN2(MAX_NINIO, (nl - ns) * P->ninio[2]);
          energy += P->mismatch1nI[type][si1][sj1] +
                    P->mismatch1nI[type_2][sq1][sp1];
        }

        break;

      case 2:
        if (nl == 2) {
          /* 2x2 loop */
          energy = P->int22[type][type_2][si1][sp1][sq1][sj1];
          break;
        } else if (nl == 3) {
          /* 2x3 loop */
          energy = P->internal_loop[5] +
                   P->ninio[2];
          energy += P->mismatch23I[type][si1][sj1] +
                    P->mismatch23I[type_2][sq1][sp1];
          break;
        }
        /* fall through */

      default:
        /* generic interior loop (no else here!)*/
        u       = nl + ns;
        energy += (u <= MAXLOOP) ?
                  (P->internal_loop[u]) :
                  (P->internal_loop[30] + (int)(P->lxc * log((u) / 30.)));

        energy += MIN2(MAX_NINIO, (nl - ns) * P->ninio[2]);

        energy += P->mismatchI[type][si1][sj1] +
                  P->mismatchI[type_2][sq1][sp1];

        break;
    }

    energy += salt_loop_correction;
  }

  return energy;
}


PUBLIC int
vrna_eval_internal(vrna_fold_compound_t *fc,
                   unsigned int         i,
                   unsigned int         j,
                   unsigned int         k,
                   unsigned int         l,
                   unsigned int         options)
{
  unsigned char         eval;
  eval_hc               evaluate;
  struct hc_int_def_dat hc_dat_local;

  if ((fc) &&
      (i > 0) &&
      (j > 0) &&
      (k > 0) &&
      (l > 0)) {

    /* prepare hard constraints check */
    if ((options & VRNA_EVAL_LOOP_NO_HC) ||
        (fc->hc == NULL)) {
      eval = (unsigned char)1;
    } else {
      if (fc->hc->type == VRNA_HC_WINDOW)
#if 1
        return INF;
#else        
        evaluate = prepare_hc_int_def_window(fc, &hc_dat_local);
#endif
      else
        evaluate = prepare_hc_int_def(fc, &hc_dat_local);

      eval = evaluate(i, j, k, l, &hc_dat_local);
    }

    /* is this base pair allowed to close a hairpin (like) loop ? */
    if (eval)
      return  (j < k) ?
              eval_ext_int_loop(fc, i, j, k, l, options) :
              eval_int_loop(fc, i, j, k, l, options);
  }

  return INF;
}


/*
 #####################################
 # BEGIN OF STATIC HELPER FUNCTIONS  #
 #####################################
 */
PRIVATE INLINE int
eval_int_loop(vrna_fold_compound_t  *fc,
              unsigned int          i,
              unsigned int          j,
              unsigned int          k,
              unsigned int          l,
              unsigned int          options)
{
  short             *S, *S2, **SS, **S5, **S3;
  unsigned int      *sn, n_seq, s, **a2s, u1, u2, type, type2, with_ud;
  int               e, e5, e3;
  vrna_param_t      *P;
  vrna_md_t         *md;
  vrna_ud_t         *domains_up;
  struct sc_int_dat sc_wrapper;

  n_seq       = (fc->type == VRNA_FC_TYPE_SINGLE) ? 1 : fc->n_seq;
  P           = fc->params;
  md          = &(P->model_details);
  sn          = fc->strand_number;
  S           = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding : NULL;
  S2          = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;
  SS          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  S5          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S5;
  S3          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S3;
  a2s         = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->a2s;
  domains_up  = fc->domains_up;
  with_ud     = ((domains_up) && (domains_up->energy_cb)) ? 1 : 0;
  e           = INF;

  if ((sn[i] != sn[k]) ||
      (sn[l] != sn[j]))
    return INF;

  switch (fc->type) {
    case VRNA_FC_TYPE_SINGLE:
      type  = vrna_get_ptype_md(S2[i], S2[j], md);
      type2 = vrna_get_ptype_md(S2[l], S2[k], md);

      u1  = k - i - 1;
      u2  = j - l - 1;

      /* regular interior loop */
      e = vrna_E_internal(u1, u2, type, type2, S[i + 1], S[j - 1], S[k - 1], S[l + 1], P);

      break;

    case VRNA_FC_TYPE_COMPARATIVE:
      e = 0;
      for (s = 0; s < n_seq; s++) {
        type    = vrna_get_ptype_md(SS[s][i], SS[s][j], md);
        type2   = vrna_get_ptype_md(SS[s][l], SS[s][k], md);
        u1      = a2s[s][k - 1] - a2s[s][i];
        u2      = a2s[s][j - 1] - a2s[s][l];
        e       += vrna_E_internal(u1, u2, type, type2, S3[s][i], S5[s][j], S5[s][k], S3[s][l], P);
      }

      break;
  }

  if (e != INF) {
    if (!(options & VRNA_EVAL_LOOP_NO_SC)) {
      init_sc_int(fc, &sc_wrapper);

      /* add soft constraints */
      if (sc_wrapper.pair)
        e += sc_wrapper.pair(i, j, k, l, &sc_wrapper);

      free_sc_int(&sc_wrapper);
    }

    if (with_ud) {
      int energy  = e;
      e5      = e3 = 0;

      u1  = k - i - 1;
      u2  = j - l - 1;

      if (u1 > 0) {
        e5 = domains_up->energy_cb(fc,
                                   i + 1, k - 1,
                                   VRNA_UNSTRUCTURED_DOMAIN_INT_LOOP,
                                   domains_up->data);
      }

      if (u2 > 0) {
        e3 = domains_up->energy_cb(fc,
                                   l + 1, j - 1,
                                   VRNA_UNSTRUCTURED_DOMAIN_INT_LOOP,
                                   domains_up->data);
      }

      e = MIN2(e, energy + e5);
      e = MIN2(e, energy + e3);
      e = MIN2(e, energy + e5 + e3);
    }
  }

  return e;
}


PRIVATE INLINE int
eval_ext_int_loop(vrna_fold_compound_t  *fc,
                  unsigned int          i,
                  unsigned int          j,
                  unsigned int          k,
                  unsigned int          l,
                  unsigned int          options)
{
  unsigned int      n, n_seq, s, **a2s, type, type2, with_ud, u1, u2, u3;
  int               e, energy, e5, e3;
  short             *S, *S2, **SS, **S5, **S3;
  vrna_param_t      *P;
  vrna_md_t         *md;
  vrna_ud_t         *domains_up;
  struct sc_int_dat sc_wrapper;

  n           = fc->length;
  n_seq       = (fc->type == VRNA_FC_TYPE_SINGLE) ? 1 : fc->n_seq;
  P           = fc->params;
  md          = &(P->model_details);
  S           = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding : NULL;
  S2          = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;
  SS          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  S5          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S5;
  S3          = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S3;
  a2s         = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->a2s;
  domains_up  = fc->domains_up;
  with_ud     = ((domains_up) && (domains_up->energy_cb)) ? 1 : 0;
  e           = INF;

  init_sc_int(fc, &sc_wrapper);

  {
    energy = 0;

    switch (fc->type) {
      case VRNA_FC_TYPE_SINGLE:
        type  = vrna_get_ptype_md(S2[j], S2[i], md);
        type2 = vrna_get_ptype_md(S2[l], S2[k], md);

        u1  = i - 1;
        u2  = k - j - 1;
        u3  = n - l;

        /* regular interior loop */
        energy = vrna_E_internal(u2, u1 + u3, type, type2, S[j + 1], S[i - 1], S[k - 1], S[l + 1], P);

        break;

      case VRNA_FC_TYPE_COMPARATIVE:
        for (s = 0; s < n_seq; s++) {
          type    = vrna_get_ptype_md(SS[s][j], SS[s][i], md);
          type2   = vrna_get_ptype_md(SS[s][l], SS[s][k], md);
          u1      = a2s[s][i - 1];
          u2      = a2s[s][k - 1] - a2s[s][j];
          u3      = a2s[s][n] - a2s[s][l];
          energy  += vrna_E_internal(u2, u1 + u3, type, type2, S3[s][j], S5[s][i], S5[s][k], S3[s][l], P);
        }

        break;
    }

    /* add soft constraints */
    if (sc_wrapper.pair_ext)
      energy += sc_wrapper.pair_ext(i, j, k, l, &sc_wrapper);

    e = energy;

    if (with_ud) {
      e5 = e3 = 0;

      u1  = i - 1;
      u2  = k - j - 1;
      u3  = n - l;

      if (u2 > 0) {
        e5 = domains_up->energy_cb(fc,
                                   j + 1, k - 1,
                                   VRNA_UNSTRUCTURED_DOMAIN_INT_LOOP,
                                   domains_up->data);
      }

      if (u1 + u3 > 0) {
        e3 = domains_up->energy_cb(fc,
                                   l + 1, i - 1,
                                   VRNA_UNSTRUCTURED_DOMAIN_INT_LOOP,
                                   domains_up->data);
      }

      e = MIN2(e, energy + e5);
      e = MIN2(e, energy + e3);
      e = MIN2(e, energy + e5 + e3);
    }
  }

  free_sc_int(&sc_wrapper);

  return e;
}


/*
 *###########################################
 *# deprecated functions below              #
 *###########################################
 */

#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY

PUBLIC int
vrna_eval_int_loop(vrna_fold_compound_t *fc,
                   int                  i,
                   int                  j,
                   int                  k,
                   int                  l)
{
  return vrna_eval_internal(fc,
                            (unsigned int)i,
                            (unsigned int)j,
                            (unsigned int)k,
                            (unsigned int)l,
                            VRNA_EVAL_LOOP_NO_HC);
}

#endif
