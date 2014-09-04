#include "m_pd.h"
#include <math.h>

static t_class *ceff_tilde_class;

typedef struct lopctl /* structure pour le besoin du filtre*/
{
    t_sample c_x; /*sample en mémoire*/
    t_sample c_coef; /* relatif a la frequence de coupure*/
} t_lopctl;

typedef struct _ceff_tilde
{
    t_object x_obj;
    t_float x_hz; /* frequence de coupure du filtre*/
    t_float x_sr; /* Sample rate*/
    t_float x_f; /* Sample en entree*/
    t_lopctl x_cspace; /* structure pour le besoin du filtre*/
    t_lopctl *x_ctl; /* pointeur vers la structure*/
} t_ceff_tilde;

void ceff_fc(t_ceff_tilde *x, t_floatarg f);

void *ceff_tilde_new(t_float drive, t_float freqc)
{
    t_ceff_tilde *x = (t_ceff_tilde *)pd_new(ceff_tilde_class);
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), drive); /* deuxieme inlet, controle de la distortion*/
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), freqc); /*troisieme inlet, controle du filtre passe bas */
    outlet_new(&x->x_obj, &s_signal);
    x->x_sr = 44100; /* recalculee tout les vecteurs*/
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x = 0; /* valeur simulee du dernier sample*/
    x->x_f = 0;
    ceff_fc(x, freqc); /* calcul du coefficient proportionel a la frequence de coupure*/
    
    return (x);
}

void ceff_fc(t_ceff_tilde *x, t_floatarg f) /* fonction pour le calcul du coeff par la frequence de coupure*/
{
    if (f < 0) f = 0;
    x->x_hz = f;
    x->x_ctl->c_coef = f * (2 * 3.14159) / x->x_sr; /* f est ensuite ramene entre entre 0 et ~7018 Hz*/
    if (x->x_ctl->c_coef > 1)
        x->x_ctl->c_coef = 1;
    else if (x->x_ctl->c_coef < 0)
        x->x_ctl->c_coef = 0;
}

t_int *ceff_tilde_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *in3 = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    t_lopctl *c = (t_lopctl *)(w[5]);
    int n = (int)(w[6]);
    t_ceff_tilde *x = (t_ceff_tilde*)(w[7]);
    int i;
    t_sample last = c->c_x;
    t_sample coef = c->c_coef;
    t_sample feedback = 1 - coef;
    t_sample sortie;
    
    for (i = 0; i < n; i++) /* Pour tout les echantillons du vecteur*/
    {
        float base = *in1++;
        float puissance = *in2++;
        
        ceff_fc(x,*in3++); /* controle en audio de la frequence du coupure*/
        coef = c->c_coef;
        feedback = 1 - coef;
        
        if (puissance>1) /* Clip entre 0 et 1*/
            puissance=1;
        else if (puissance<0)
            puissance=0;
        
        if (base<0) /* application de la distortion*/
            sortie = -pow(fabsf(base),(1-puissance));
        else
            sortie = pow(base,(1-puissance));
        
        last = *out++ = coef * sortie + feedback * last; /* application du filtre*/
        if (PD_BIGORSMALL(last)) /* test de grandeur*/
            last = 0;
        c->c_x = last;
    }
    return (w+8);
}

void ceff_tilde_dsp(t_ceff_tilde *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr; /* samplerate actuel d'un vecteur*/
    /*ceff_fc(x,  x->x_hz);  actif dans de le cas ou la freq de coupure nest pas reglable en audio*/
    dsp_add(ceff_tilde_perform, 7,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, x->x_ctl, sp[0]->s_n, x); /* pointeurs utiles*/
}

void ceff_tilde_setup(void) {
    ceff_tilde_class = class_new(gensym("ceff~"), (t_newmethod)ceff_tilde_new, 0, sizeof(t_ceff_tilde), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(ceff_tilde_class, t_ceff_tilde, x_f);
    class_addmethod(ceff_tilde_class, (t_method)ceff_tilde_dsp, gensym("dsp"), 0);
}
