#ifndef __DEAD_LOCK_STUB_H__
#define __DEAD_LOCK_STUB_H__

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <pthread.h>

#include <deque>
#include <vector>
#include <map>

#include "common/log_proc.inc"

struct thread_graphic_vertex_t
{
    int indegress;
    std::vector<uint64_t> vertexs;

    thread_graphic_vertex_t()
        : indegress(0)
    {
    }
};

class DeadLockGraphic
{

public:
    static DeadLockGraphic &getInstance()
    {
        static DeadLockGraphic instance;
        return instance;
    }

    void lock_before(uint64_t thread_id, uint64_t lock_addr)
    {
        pthread_mutex_lock(&m_mutex);
        //(A) m_thread_apply_lock, ��� thread_id => lock_addr
        m_thread_apply_lock[thread_id] = lock_addr;
        pthread_mutex_unlock(&m_mutex);
    }

    void lock_after(uint64_t thread_id, uint64_t lock_addr)
    {
        pthread_mutex_lock(&m_mutex);
        //(B)m_thread_apply_lock, ȥ�� thread_id => lock_addr
        m_thread_apply_lock.erase(thread_id);

        //(A)m_lock_belong_thread, add lock_addr => thread_id
        m_lock_belong_thread[lock_addr] = thread_id;
        pthread_mutex_unlock(&m_mutex);
    }

    void unlock_after(uint64_t thread_id, uint64_t lock_addr)
    {
        pthread_mutex_lock(&m_mutex);
        //(B)m_lock_belong_thread, remove lock_addr => thread_id
        m_lock_belong_thread.erase(lock_addr);
        pthread_mutex_unlock(&m_mutex);
    }

    void check_dead_lock()
    {
        std::map<uint64_t, uint64_t> lock_belong_thread;
        std::map<uint64_t, uint64_t> thread_apply_lock;

        pthread_mutex_lock(&m_mutex);
        lock_belong_thread = m_lock_belong_thread;
        thread_apply_lock = m_thread_apply_lock;
        pthread_mutex_unlock(&m_mutex);

        //��������ͼ
        std::map<uint64_t, thread_graphic_vertex_t> graphics;

        for (std::map<uint64_t, uint64_t>::const_iterator iter = m_thread_apply_lock.begin();
             iter != m_thread_apply_lock.end(); iter++)
        {
            uint64_t thd_id1 = iter->first;
            uint64_t lock_id = iter->second;

            if (m_lock_belong_thread.find(lock_id) == m_lock_belong_thread.end())
            {
                continue;
            }

            uint64_t thd_id2 = m_lock_belong_thread[lock_id];

            if (graphics.find(thd_id1) == graphics.end())
            {
                graphics[thd_id1] = thread_graphic_vertex_t();
            }

            if (graphics.find(thd_id2) == graphics.end())
            {
                graphics[thd_id2] = thread_graphic_vertex_t();
            }

            //���������
            graphics[thd_id1].vertexs.push_back(thd_id2);
            //��� indegress++
            graphics[thd_id2].indegress++;
        }

        //�������һ
        uint64_t counter = 0;
        std::deque<uint64_t> graphics_queue;

        for (std::map<uint64_t, thread_graphic_vertex_t>::const_iterator iter = graphics.begin();
             iter != graphics.end(); iter++)
        {
            uint64_t thd_id = iter->first;
            const thread_graphic_vertex_t &gvert = iter->second;

            if (gvert.indegress == 0)
            {
                graphics_queue.push_back(thd_id);
                counter ++;
            }
        }

        //������̶�
        while (!graphics_queue.empty())
        {
            uint64_t thd_id = graphics_queue.front();
            graphics_queue.pop_front();

            const thread_graphic_vertex_t &gvert = graphics[thd_id];

            //�����ڽ������
            for (size_t i = 0; i < gvert.vertexs.size(); i++)
            {
                uint64_t thd_id2 = gvert.vertexs[i];
                graphics[thd_id2].indegress --;

                if (graphics[thd_id2].indegress == 0)
                {
                    graphics_queue.push_back(thd_id2);
                    counter++;
                }
            }
        }

        //���������
        if (counter != graphics.size())
        {
            printf("��⵽��������, �����Զ�����!!!!!!!!!!!!\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "��⵽��������, �����Զ�����!!!!!!!!!!!!\n");
            osip_usleep(5000000);
            system((char*)"killall cms; killall -9 cms");
        }
        else
        {
            //printf("û�м�⵽����.\n");
        }

    }

    void start_check()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, thread_rountine, (void *)(this));
    }

    static void *thread_rountine(void *args)
    {
        DeadLockGraphic *ptr_graphics = static_cast<DeadLockGraphic *>(args);

        while (true)
        {
            //ÿʮ����һ��
            sleep(10);
            ptr_graphics->check_dead_lock();
        }
    }

private:

    //lock ��Ӧ �߳� ӵ���ߵ�map
    std::map<uint64_t, uint64_t> m_lock_belong_thread;

    //�̳߳���ȥ�����lock map
    std::map<uint64_t, uint64_t> m_thread_apply_lock;

    pthread_mutex_t m_mutex;


private:
    DeadLockGraphic()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }
    ~DeadLockGraphic()
    {
        pthread_mutex_destroy(&m_mutex);
    }
private:
    DeadLockGraphic(const DeadLockGraphic &)
    {
    }
    DeadLockGraphic& operator=(const DeadLockGraphic &)
    {
        return *this;
    }

private:


};

#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

//����lock, ���before, after����, ��¼�����̵߳Ĺ�ϵ
#define pthread_mutex_lock(x)                                                                       \
    do {                                                                                            \
        DeadLockGraphic::getInstance().lock_before(gettid(), reinterpret_cast<uint64_t>(x));        \
        pthread_mutex_lock(x);                                                                      \
        DeadLockGraphic::getInstance().lock_after(gettid(), reinterpret_cast<uint64_t>(x));         \
    } while (false);

//����unlock, ���after����, ��������̵߳Ĺ�ϵ
#define pthread_mutex_unlock(x)                                                                     \
    do {                                                                                            \
        pthread_mutex_unlock(x);                                                                    \
        DeadLockGraphic::getInstance().unlock_after(gettid(), reinterpret_cast<uint64_t>(x));       \
    } while(false);

#endif
