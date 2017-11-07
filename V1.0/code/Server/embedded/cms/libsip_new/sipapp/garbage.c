#include "garbage.inc"

garbage_t * g_recv_garbage = NULL;               /* sip: recv garbage */
garbage_t * g_recv_register_garbage = NULL;      /* sip: recv register garbage */
garbage_t * g_recv_msg_garbage = NULL;           /* sip: recv msg garbage */
garbage_t * g_send_garbage = NULL;               /* sip: send garbage */
garbage_t * g_send_msg_garbage = NULL;           /* sip: send msg garbage */

int create_garbage(garbage_t** garbage)
{
    (*garbage) = (garbage_t*)osip_malloc(sizeof(garbage_t));

    if (NULL == (*garbage))
    {
        return -1;
    }

    /*
     * init garbage list
     */
    (*garbage)->garbage = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*garbage)->garbage)
    {
        osip_free(*garbage);
        *garbage = NULL;

        return -1;
    }

    osip_list_init((*garbage)->garbage);

    /*
     * init smutex
     */
#ifdef MULTI_THR
    (*garbage)->gislocked = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*garbage)->gislocked)
    {
        osip_free((*garbage)->garbage);
        (*garbage)->garbage = NULL;

        osip_free(*garbage);
        *garbage = NULL;

        return -1;
    }

#endif
    /*
    * OK , Init done
    */
    return 0;

}

void destroy_garbage(garbage_t* garbage)
{
    if (NULL != garbage)
    {
        if (0 != garbage->garbage->nb_elt)
        {
            clean_garbage(garbage);
        }

#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)garbage->gislocked);
        garbage->gislocked = NULL;
#endif
        osip_free(garbage->garbage);
        garbage->garbage = NULL;

        osip_free(garbage);
        garbage = NULL;
    }
}

int throw_2garbage(garbage_t* garbage, osip_transaction_t* trans)
{
    if (NULL == garbage)
    {
        return -1;
    }

#ifdef MULTI_THR
    /* lock it !!!! */
    osip_mutex_lock((struct osip_mutex*)garbage->gislocked);
#endif
    /*
     * OK, locked and then add a transaction to garbage list
     */
    osip_list_add(garbage->garbage, (void*)trans, -1);

#ifdef MULTI_THR
    /* unlock it !!! */
    osip_mutex_unlock((struct osip_mutex*)garbage->gislocked);
#endif

    return 0;

}

int clean_garbage(garbage_t* garbage)
{
    osip_transaction_t* trans = NULL;

    if (garbage == NULL)
    {
        return -1;
    }

#ifdef MULTI_THR
    /* lock it!!! */
    osip_mutex_lock((struct osip_mutex*)garbage->gislocked);
#endif

    while (!osip_list_eol(garbage->garbage, 0))
    {
        trans = (osip_transaction_t*)osip_list_get(garbage->garbage, 0);

        if (NULL != trans)
        {
            osip_list_remove(garbage->garbage, 0);
            osip_transaction_free2(trans);
            trans = NULL;
        }
    }

#ifdef MULTI_THR
    /* unlock it !!! */
    osip_mutex_unlock((struct osip_mutex*)garbage->gislocked);
#endif
    return 0;

}

void ThrowAll2Garbage(osip_t* cell, garbage_t* garbage)
{
    int pos = 0;
    osip_transaction_t* transaction = NULL;

    if ((NULL == cell) || (NULL == garbage))
    {
        return;
    }

    /*remove ICT */
    while (!osip_list_eol(&cell->osip_ict_transactions, pos))
    {
        transaction = (osip_transaction_t*)osip_list_get(&cell->osip_ict_transactions, pos);

        if (transaction)
        {
            osip_list_remove(&cell->osip_ict_transactions, pos);
            throw_2garbage(garbage, transaction);
        }
    }

    /*remove IST */
    pos = 0;

    while (!osip_list_eol(&cell->osip_ist_transactions, pos))
    {
        transaction = (osip_transaction_t*)osip_list_get(&cell->osip_ist_transactions, pos);

        if (transaction)
        {
            osip_list_remove(&cell->osip_ist_transactions, pos);
            throw_2garbage(garbage, transaction);
        }
    }


    /*remove NICT */
    pos = 0;

    while (!osip_list_eol(&cell->osip_nict_transactions, pos))
    {
        transaction = (osip_transaction_t*)osip_list_get(&cell->osip_nict_transactions, pos);

        if (transaction)
        {
            osip_list_remove(&cell->osip_nict_transactions, pos);
            throw_2garbage(garbage, transaction);
        }
    }

    /*remove NIST */
    pos = 0;

    while (!osip_list_eol(&cell->osip_nist_transactions, pos))
    {
        transaction = (osip_transaction_t*)osip_list_get(&cell->osip_nist_transactions, pos);

        if (transaction)
        {
            osip_list_remove(&cell->osip_nist_transactions, pos);
            throw_2garbage(garbage, transaction);
        }
    }
}

