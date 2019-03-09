#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>

/*
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/*
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to
 * declare other global variables if your solution requires them.
 */


static struct array* intersection;
static struct array* wait;
static struct lock* lk;
static struct cv* cv_e;
static struct cv* cv_n;
static struct cv* cv_w;
static struct cv* cv_s;
static struct cv* cv;
volatile int emergency;


typedef struct Vehicles
{
    Direction origin;
    Direction destination;
} Vehicle;

int match(void* arr, Vehicle* v);
int turnRight(Direction origin, Direction destination);



void
intersection_sync_init(void)
{
        lk = lock_create("lock");
        cv_s = cv_create("s");
        cv_w = cv_create("w");
        cv_n = cv_create("n");
        cv_e = cv_create("e");
        cv = cv_create("cv");
        emergency=0;
        intersection = array_create();
        wait = array_create();

        if((intersection == NULL)||(wait == NULL)||(lk == NULL)||
           (cv_s == NULL)||(cv_w == NULL)||(cv_n == NULL)||(cv_e == NULL)||(cv == NULL)){
                panic("create failure");
        }
}

/*
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
        KASSERT((intersection!=NULL)&&(lk!=NULL)&&(cv_e!=NULL)&&
                (cv_n!=NULL)&&(cv_w!=NULL)&&(cv_s!=NULL)&&(cv!=NULL)&&(wait!=NULL));

        lock_destroy(lk);

        cv_destroy(cv_s);
        cv_destroy(cv_w);
        cv_destroy(cv_n);
        cv_destroy(cv_e);
        cv_destroy(cv);

        int n = array_num(wait);

        ////////
        if(n!=0){for(int i = 0; i < n; i++){array_remove(wait, (unsigned int) 0);} }//for debug purpose
        ////////
        array_destroy(wait);
        array_destroy(intersection);

}


int turnRight(Direction origin, Direction destination){
        if(origin==east){
                if(destination==north)return 1;
        }else if(origin==south){
                if(destination==east)return 1;
        }else if(origin==west){
                if(destination==south)return 1;
        }else if(origin==north){
                if(destination==west)return 1;
        }
        return 0;
}

//test whether compatible with the intersection
int match(void* arr, Vehicle* v){
        int m = 1;
        Vehicle* tmp = NULL;
        int n = (int) array_num(arr);
        if(n==0) return m;
        for(int i=0; i<n; i++){
                //whether go against anyone in the array
                tmp = array_get(arr, i);
                KASSERT(tmp!=NULL);

                if(((tmp->destination != v->origin)||//cond1
                    (tmp->origin != v->destination))

                   &&(tmp->origin != v->origin)//cond2

                   //cond3
                   &&( ((turnRight(tmp->origin, tmp->destination)==0)&&(turnRight(v->origin, v->destination)==0))
                       ||(tmp->destination == v->destination) )   ) {
                        m = 0;
                        break;
                }
        }
        return m;
}




void
intersection_before_entry(Direction origin, Direction destination)
{
        /* replace this default implementation with your own implementation */
        lock_acquire(lk);

        Vehicle* v = kmalloc(sizeof(Vehicle));
        KASSERT((destination==south)||(destination==north)||(destination==east)||(destination==west));
        KASSERT((origin==south)||(origin==north)||(origin==east)||(origin==west));

        v->destination = destination;
        v->origin = origin;

        int m = match(intersection, v);

        ///see if you can go
        //1. turn right
        //2. (m == 1) arr is empty
        //3. (m == 1) ok with all the existing thread in the array
        // aka either same origin or same destination
        if(m){
                int s = array_add(intersection, v, NULL);
                KASSERT(s==0);
        }else{
                int s = array_add(wait, v, NULL);
                KASSERT(s==0);
                //You will have to wait until being called
                if(origin == north){
                        if(match(intersection, v)==0)cv_wait(cv_n, lk);
                }else if(origin == east){
                        if(match(intersection, v)==0)cv_wait(cv_e, lk);
                }else if(origin == south){
                        if(match(intersection, v)==0)cv_wait(cv_s, lk);
                }else if(origin == west){
                        if(match(intersection, v)==0)cv_wait(cv_w, lk);
                }else{
                        panic("unexpected direction.");
                }
                if(match(intersection, v)==0){
                        emergency++;
                        while(match(intersection, v)==0){cv_wait(cv, lk);}
                        emergency--;
                }

                int i = 0;
                int n = (int) array_num(wait);
                Vehicle* out = NULL;
                for(;i<n;i++){//try to find this vehicle from array
                        out = array_get(wait, (unsigned int) i);
                        Direction d1 = origin;
                        Direction d2 = destination;
                        if(((signed int) out->origin == (int) d1)&&((signed int) out->destination == (int) d2)){
                                array_remove(wait, (unsigned int) i);
                                break;
                        }
                }
                s = array_add(intersection, v, NULL);
                KASSERT(s==0);
        }
        lock_release(lk);
        return;
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination)
{
        lock_acquire(lk);
        Vehicle* out = NULL;
        int des;
        int or = -1;
        int n = (int) array_num(intersection);
        int i = 0;
        for(;i<n;i++){//try to find this vehicle from array
                out = array_get(intersection, (unsigned int) i);
                or = out->origin;
                des = out->destination;
                Direction d1 = origin;
                Direction d2 = destination;
                if((or == (int) d1)&&(des == (int) d2)){
                        array_remove(intersection, (unsigned int) i);
                        kfree(out);
                        break;
                }
        }

        if(emergency>0){
                cv_broadcast(cv, lk);
        }else {
                int m = array_num(wait);
                Vehicle *v = NULL;
                if (m > (signed) 0) {
                        v = array_get(wait, (unsigned int) 0);
                        if (v->origin == north) {
                                cv_broadcast(cv_n, lk);
                        } else if (v->origin == east) {
                                cv_broadcast(cv_e, lk);
                        } else if (v->origin == south) {
                                cv_broadcast(cv_s, lk);
                        } else if (v->origin == west) {
                                cv_broadcast(cv_w, lk);
                        } else {
                                //exception (should not run here)
                                cv_broadcast(cv_n, lk);
                                cv_broadcast(cv_s, lk);
                                cv_broadcast(cv_e, lk);
                                cv_broadcast(cv_w, lk);
                        }

                }
        }
        lock_release(lk);
        return;
}

