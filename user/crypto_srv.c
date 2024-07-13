#include "kernel/types.h"
#include "user/user.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"

#include "kernel/crypto.h"

#define REASONABLE_DATA_SIZE 1024
#define REASONABLE_KEY_SIZE 16

int main(void)
{
  if (open("console", O_RDWR) < 0)
  {
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0); // stdout
  dup(0); // stderr

  printf("crypto_srv: starting\n");

  // TODO: implement the cryptographic server here
  if (getpid() != 2)
  {
    fprintf(2, "crypto_srv: must be started by init\n");
    exit(1);
  }

  struct crypto_op *op;
  void *addr;
  uint64 size;

  for (;;)
  {
    if (take_shared_memory_request(&addr, &size) < 0)
    {
      continue;
    }

    op = (struct crypto_op *)addr;

    // Check if the operation is valid
    if (op->state != CRYPTO_OP_STATE_INIT ||                                                                              // Check if the state is initialized
        (op->type != CRYPTO_OP_TYPE_ENCRYPT && op->type != CRYPTO_OP_TYPE_DECRYPT) ||                                     // Check if the type is valid
        op->data_size > REASONABLE_DATA_SIZE || op->data_size <= 0 || op->data_size > size - sizeof(struct crypto_op) || // Check if the data size is valid
        op->key_size > REASONABLE_KEY_SIZE)                                                                               // Check if the key size is valid
    {
      op->state = CRYPTO_OP_STATE_ERROR;
    }
    else
    {

      if (op->key_size > 0)
      {
        // Decrypt or Encrypt the payload
        for (int i = 0; i < op->data_size; i++)
        {
          op->payload[i + op->key_size] ^= op->payload[i % op->key_size];
        }
      }

      // Memory barrier (fence) to ensure that the data is written back to memory before setting the state to done
      asm volatile("fence rw,rw" : : : "memory");

      // Set the state to done
      op->state = CRYPTO_OP_STATE_DONE;
    }

    memcpy(addr, op, sizeof(struct crypto_op));
    if (remove_shared_memory_request(addr, size) < 0)
    {
      fprintf(2, "crypto_srv: failed to remove shared memory request\n");
      exit(1);
    }
  }

  exit(0);
}
