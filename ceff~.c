#include "m_pd.h"
#include <math.h>

static t_class *ceff_tilde_class;

typedef struct _ceff_tilde
{
    t_object x_obj;
    
    t_float f_drive; /* Sample en entree*/
} t_ceff_tilde;

void *ceff_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_ceff_tilde *x = (t_ceff_tilde *)pd_new(ceff_tilde_class); 
    
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal); /* deuxieme inlet, controle de la distortion*/
    
    outlet_new(&x->x_obj, &s_signal);
    
    x->f_drive = 0;
    
    return (x);
}

t_int *ceff_tilde_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    
    while (n--) /* Pour tout les echantillons du vecteur*/
    {
        float base = *in1++;
        float puissance = *in2++;
        
        if (puissance>1) /* Clip entre 0 et 1*/
            puissance=1;
        else if (puissance<0)
            puissance=0;
        
        if (base<0)
            *out = -pow(fabsf(base),(1-puissance));
        else
            *out = pow(base,(1-puissance));
        out++;
    }
    return (w+5);
}

void ceff_tilde_dsp(t_ceff_tilde *x, t_signal **sp)
{
    dsp_add(ceff_tilde_perform, 4,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n); /* pointeurs utiles*/
}

void ceff_tilde_setup(void) {
    ceff_tilde_class = class_new(gensym("ceff~"), (t_newmethod)ceff_tilde_new, 0, sizeof(t_ceff_tilde), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(ceff_tilde_class, t_ceff_tilde, f_drive);
    class_addmethod(ceff_tilde_class, (t_method)ceff_tilde_dsp, gensym("dsp"), 0);
}
