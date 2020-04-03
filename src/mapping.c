#include <stdio.h>
#include <string.h>
#include "mapping.h"

#define test_bit(bit, array) (array[bit / 8] & (1 << (bit % 8)))

int processMaps(Mapping *m)
{
  for (int i = 0; i < m->insideCount; i++)
  {
    switch (m->insideMappings[i].type)
    {
    case ABS:
      m->analogueMapping[m->insideMappings[i].channel] = findMapping(m->insideMappings[i].mode, m);
      m->analogueMapping[m->insideMappings[i].channel].min = m->insideMappings[i].min;
      m->analogueMapping[m->insideMappings[i].channel].max = m->insideMappings[i].max;
      m->analogueMapping[m->insideMappings[i].channel].reverse = m->insideMappings[i].reverse;
      break;
    case KEY:
      m->keyMapping[m->insideMappings[i].channel] = findMapping(m->insideMappings[i].mode, m);
      break;
    default:
      printf("Mapping.c: Unknown inside mapping case\n");
    }
  }
}

MappingOut findMapping(Mode mode, Mapping *m)
{
  for (int i = 0; i < m->outsideCount; i++)
  {
    if (m->outsideMappings[i].mode == mode)
    {
      return m->outsideMappings[i];
    }
  }
  printf("Error: No arcade map doesn't support %s\n", modeEnumToString(mode));
  return m->outsideMappings[1];
}

void printMapping(Mapping *m)
{
  printf("Buttons\n");
  for (int i = 0; i < MAX_EV_ITEMS; i++)
  {
    if (m->keyMapping[i].type != NONE)
    {
      printf("\tEV_KEY %d -> %d (%d, %s)\n", i, m->keyMapping[i].channel, m->keyMapping[i].type, modeEnumToString(m->keyMapping[i].mode));
    }
  }
  printf("Analogue\n");
  for (int i = 0; i < MAX_EV_ITEMS; i++)
  {
    if (m->analogueMapping[i].type != NONE)
    {
      printf("\tEV_ABS %d [%d -> %d] -> %d  (%d, %s)\n", i, m->analogueMapping[i].min, m->analogueMapping[i].max, m->analogueMapping[i].channel, m->analogueMapping[i].type, modeEnumToString(m->analogueMapping[i].mode));
    }
  }
}

pthread_t threadID[256];
int threadCount = 0;
int threadsRunning = 1;

int startThread(char *eventPath, char *mappingPathIn, char *mappingPathOut)
{
  struct MappingThreadArguments *args = malloc(sizeof(struct MappingThreadArguments));
  strcpy(args->eventPath, eventPath);
  strcpy(args->mappingPathIn, mappingPathIn);
  strcpy(args->mappingPathOut, mappingPathOut);
  pthread_create(&threadID[threadCount], NULL, deviceThread, args);
  threadCount++;
}

void stopThreads()
{
  printf("Stopping threads\n");
  threadsRunning = 0;
  for (int i = 0; i < threadCount; i++)
  {
    pthread_join(threadID[i], NULL);
  }
}

void *deviceThread(void *_args)
{
  struct MappingThreadArguments *args = (struct MappingThreadArguments *)_args;
  char eventPath[4096];
  char mappingPathIn[4096];
  char mappingPathOut[4096];

  strcpy(eventPath, args->eventPath);
  strcpy(mappingPathIn, args->mappingPathIn);
  strcpy(mappingPathOut, args->mappingPathOut);

  free(args);

  Mapping m;

  m.insideCount = processInMapFile(mappingPathIn, m.insideMappings);
  m.outsideCount = processOutMapFile(mappingPathOut, m.outsideMappings);

  if ((m.deviceFd = open(eventPath, O_RDONLY)) == -1)
  {
    printf("mapping.c:initDevice(): Failed to open device file descriptor\n");
    exit(-1);
  }

  processMaps(&m);

  struct input_event event;

  int flags = fcntl(m.deviceFd, F_GETFL, 0);
  fcntl(m.deviceFd, F_SETFL, flags | O_NONBLOCK);

  int axisIndex;
  uint8_t absoluteBitmask[ABS_MAX / 8 + 1];
  float percentageDeadzone;
  struct input_absinfo absoluteFeatures;

  memset(absoluteBitmask, 0, sizeof(absoluteBitmask));
  if (ioctl(m.deviceFd, EVIOCGBIT(EV_ABS, sizeof(absoluteBitmask)), absoluteBitmask) < 0)
  {
    perror("evdev ioctl");
  }

  for (axisIndex = 0; axisIndex < ABS_MAX; ++axisIndex)
  {
    if (test_bit(axisIndex, absoluteBitmask))
    {
      if (ioctl(m.deviceFd, EVIOCGABS(axisIndex), &absoluteFeatures))
      {
        perror("evdev EVIOCGABS ioctl");
      }
      m.analogueMapping[axisIndex].max = absoluteFeatures.maximum;
      m.analogueMapping[axisIndex].min = absoluteFeatures.minimum;
    }
  }

  //printMapping(&m);

  while (threadsRunning)
  {
    if (read(m.deviceFd, &event, sizeof event) > 0)
    {
      //controlPrintStatus();

      switch (event.type)
      {
      case EV_ABS:
        if (m.analogueMapping[event.code].type != NONE)
        {

          float x = event.value;
          float min = m.analogueMapping[event.code].min;
          float max = m.analogueMapping[event.code].max;
          if (m.analogueMapping[event.code].reverse)
          {
            float temp = min;
            min = max;
            max = temp;
          }
          int scaled = (int)((float)(x - min) / (float)(max - min) * 255);

          //printf("analogue (min %d, max %d, raw %d) %d -> %d\n", m.analogueMapping[event.code].min, m.analogueMapping[event.code].max, event.value, m.analogueMapping[event.code].channel, scaled);
          if (m.analogueMapping[event.code].type == ANALOGUE)
          {
            setAnalogue(m.analogueMapping[event.code].channel, scaled);
          }
          else if (m.analogueMapping[event.code].type == ROTARY)
          {
            setRotary(m.analogueMapping[event.code].channel, scaled);
          }
        }
        break;
      case EV_KEY:
        if (m.keyMapping[event.code].type != NONE)
        {
          if (event.value != 2)
          {
            if (m.keyMapping[event.code].type == BUTTON)
            {
              setSwitch(1, m.keyMapping[event.code].channel, event.value);
              setSwitch(2, m.keyMapping[event.code].channel, event.value);
            }
            else if (m.keyMapping[event.code].type == SYSTEM)
            {
              setSwitch(0, m.keyMapping[event.code].channel, event.value);
            }
            //printf("key %d -> %d\n", m.keyMapping[event.code].channel, event.value);
          }
        }

        break;
      }
    }
  }

  printf("Closing\n");
  close(m.deviceFd);

  return 0;
}
