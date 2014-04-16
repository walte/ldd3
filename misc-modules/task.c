/*                                                     
 * $Id: task.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0:""0.1");

static int test_thread(void *data) 
{
	while(1)
	{
		printk(KERN_ERR "%d, %d, %d\n", current->prio, current->static_prio, current->rt_priority);
		mdelay(1000);
	}
}

static int task_init(void)
{
	struct task_struct *task;
	struct task_struct *t;

	struct task_struct *te;

	printk(KERN_ALERT "Hello, world\n");

	for_each_process(task) {
		printk("%s (pid = %d, %d, %d), %s (pid %d, %d), %s (gl %d, %d)\n", task->comm, task_pid_nr(task), task->static_prio, task->rt_priority,
				task->parent->comm, task_pid_nr(task->parent), task->parent->static_prio,
				task->group_leader->comm, task_pid_nr(task->group_leader), task->group_leader->static_prio);
				/* task->real_parent->comm, task_pid_nr(task->real_parent)); */

		if (!thread_group_empty(task)) {
			for (t = next_thread(task); t != task; t = next_thread(t)) {
				printk("thread group %s (pid = %d, %d, %d), %s (gl %d, %d)\n", t->comm, task_pid_nr(t), t->static_prio, t->rt_priority,
						t->group_leader->comm, task_pid_nr(t->group_leader), t->group_leader->static_prio);
			}
		}
	}

	te = kthread_create(test_thread, NULL, "test");	
	wake_up_process(te);

	return 0;
}

static void task_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(task_init);
module_exit(task_exit);
