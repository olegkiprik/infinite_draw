#include <SFML/Config.h>
#include <SFML/Graphics/Color.h>
#include <SFML/Graphics/PrimitiveType.h>
#include <SFML/Graphics/RenderWindow.h>
#include <SFML/Graphics/Types.h>
#include <SFML/Graphics/Vertex.h>
#include <SFML/Graphics/VertexBuffer.h>
#include <SFML/Graphics/View.h>
#include <SFML/System/Clock.h>
#include <SFML/System/InputStream.h>
#include <SFML/System/Types.h>
#include <SFML/System/Vector2.h>
#include <SFML/Window/Clipboard.h>
#include <SFML/Window/Cursor.h>
#include <SFML/Window/Event.h>
#include <SFML/Window/Mouse.h>
#include <SFML/Window/Types.h>
#include <SFML/Window/VideoMode.h>
#include <SFML/Window/Window.h>
#include <SFML/Window/WindowBase.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(WIN32)
#include <io.h>
#define F_OK 0
#define access _access
#else
#if defined(__GNUC__)
#include <unistd.h>
#endif
#endif

#define lmin(x, y) (((x) < (y)) ? (x) : (y))
#define lmax(x, y) (((x) < (y)) ? (y) : (x))

union FloatUintConversion {
  float fl;
  uint_least32_t ui;
};

int whateverEvent(int wait, sfRenderWindow *window, sfEvent *evt, int *enough) {
  if (wait) {
    *enough = 1;
    return sfRenderWindow_waitEvent(window, evt);
  }
  return sfRenderWindow_pollEvent(window, evt);
}

#define SEC_TO_NS(sec) ((sec) * 1000000000)

int save(const sfVertex *vertices, size_t sz, sfVector2u winSize) {
  for (;;) {
    unsigned long long nanoseconds;
    struct timespec ts;
    int ret_code = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    if (ret_code == -1) {
      fprintf(stderr, "Failed to obtain timestamp. errno = %i: %s\n", errno,
              strerror(errno));
      nanoseconds = UINT64_MAX;
      return 0;
    } else {
      nanoseconds = SEC_TO_NS((unsigned long long)ts.tv_sec) +
                    (unsigned long long)ts.tv_nsec;
    }

    char *filename = (char *)malloc(50);
    if (!filename) {
      fprintf(stderr, "Failed to allocate memory (line %d)\n", __LINE__);
      return 0;
    }

    sprintf(filename, "%llu.draw", nanoseconds);
    if (access(filename, F_OK) == 0) {
      free(filename);
      continue;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
      fprintf(stderr, "Failed to open the file %s\n", filename);
      free(filename);
      return 0;
    }

    for (size_t i = 0; i < sz; ++i) {
      union FloatUintConversion xconv;
      union FloatUintConversion yconv;
      xconv.fl = vertices[i].position.x;
      yconv.fl = vertices[i].position.y;
      fprintf(f, "%u %u %u\n", xconv.ui, yconv.ui,
              sfColor_toInteger(vertices[i].color));
    }

    fclose(f);
    free(filename);
    break;
  }
  return 1;
}

int exportSvg(const sfVertex *vertices, size_t sz) {
  sfVector2f leftTop = vertices[0].position;
  sfVector2f rightBottom = vertices[0].position;

  for (size_t i = 0; i < sz; ++i) {
    leftTop.x = lmin(leftTop.x, vertices[i].position.x);
    leftTop.y = lmin(leftTop.y, vertices[i].position.y);
    rightBottom.x = lmax(rightBottom.x, vertices[i].position.x);
    rightBottom.y = lmax(rightBottom.y, vertices[i].position.y);
  }

  sfVector2u winSize = {rightBottom.x - leftTop.x + 1,
                        rightBottom.y - leftTop.y + 1};

  for (;;) {
    unsigned long long nanoseconds;
    struct timespec ts;
    int ret_code = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    if (ret_code == -1) {
      fprintf(stderr, "Failed to obtain timestamp. errno = %i: %s\n", errno,
              strerror(errno));
      nanoseconds = UINT64_MAX;
      return 0;
    } else {
      nanoseconds = SEC_TO_NS((unsigned long long)ts.tv_sec) +
                    (unsigned long long)ts.tv_nsec;
    }

    char *filename = (char *)malloc(50);
    if (!filename) {
      fprintf(stderr, "Failed to allocate memory (line %d)\n", __LINE__);
      return 0;
    }

    sprintf(filename, "%llu.html", nanoseconds);
    if (access(filename, F_OK) == 0) {
      free(filename);
      continue;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
      fprintf(stderr, "Failed to open the file %s\n", filename);
      free(filename);
      return 0;
    }

    fprintf(f,
            "<!DOCTYPE html>\n<html>\n<body "
            "style=\"background-color:#000000;\">\n<h1>svg</h1>\n"
            "<svg width=\"%d\" height=\"%d\">\n",
            winSize.x, winSize.y);

    for (size_t i = 0; i < sz; i += 2) {

      sfVector2f tr_coords_0 = {vertices[i].position.x - leftTop.x,
                                vertices[i].position.y - leftTop.y};

      sfVector2f tr_coords_1 = {vertices[i + 1].position.x - leftTop.x,
                                vertices[i + 1].position.y - leftTop.y};

      fprintf(
          f,
          "<line x1=\"%f\" y1=\"%f\" x2=\"%f"
          "\" y2=\"%f\" style=\"stroke:rgba(%d,%d,%d,%d);stroke-width:1\" />\n",
          tr_coords_0.x, tr_coords_0.y, tr_coords_1.x, tr_coords_1.y,
          (int)vertices[i].color.r, (int)vertices[i].color.g,
          (int)vertices[i].color.b, (int)vertices[i].color.a);
    }

    fprintf(f, "</svg>\n</body>\n</html>");

    fclose(f);
    free(filename);
    break;
  }
  return 1;
}

int save_to(const char *filename, const sfVertex *vertices, size_t sz,
            sfVector2u winSize) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Failed to open the file %s\n", filename);
    return 0;
  }

  for (size_t i = 0; i < sz; ++i) {
    union FloatUintConversion xconv;
    union FloatUintConversion yconv;
    xconv.fl = vertices[i].position.x;
    yconv.fl = vertices[i].position.y;
    fprintf(f, "%d %d %d\n", xconv.ui, yconv.ui,
            sfColor_toInteger(vertices[i].color));
  }

  fclose(f);
  return 1;
}

struct UndoNode {
  struct UndoNode *prev;
  size_t data;
};

void freeUndos(struct UndoNode *tail) {
  while (tail) {
    struct UndoNode *prev = tail->prev;
    free(tail);
    tail = prev;
  }
}

struct Garbage {
  struct UndoNode *undos;
  sfRenderWindow *window;
  sfClock *unredoClock;
  sfClock *zoomClock;
  sfClock *rotateClock;
  sfCursor *crossyCursor;
  sfVertexBuffer *vxb;
  sfVertex *vxa;
};

void cleanGarbage(struct Garbage g) {
  free(g.vxa);
  sfVertexBuffer_destroy(g.vxb);
  sfRenderWindow_destroy(g.window);
  sfCursor_destroy(g.crossyCursor);
  sfClock_destroy(g.unredoClock);
  sfClock_destroy(g.rotateClock);
  sfClock_destroy(g.zoomClock);
  freeUndos(g.undos);
}

int main(int argc, char **argv) {
  sfVector2u winsize = {1000, 1000};
  const size_t nrMaxVecs = 100000000;
  int preloaded = 0;
  size_t nrVcs = 0;
  size_t nrVcs2draw = 0;

  sfColor color = sfWhite;
  int ruler = 0;
  int circle = 0;

  size_t lastUpdatedNrVcs2draw = 0;

  sfVertex centerVxs[4];
  sfColor tmpCol = {160, 160, 160, 160};

  sfContextSettings settings = {0, 0, 16, 1, 1, sfContextDefault, sfFalse};

  struct Garbage g;
  memset(&g, 0, sizeof(g));

  g.vxa = (sfVertex *)malloc(nrMaxVecs * sizeof(sfVertex));
  if (!g.vxa) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  if (argc > 3) {
    FILE *fin = fopen(argv[3], "r");
    if (fin) {
      sfVector2f pos_in;
      sfUint32 color_in;
      union FloatUintConversion xconv, yconv;
      while (1) {
        if (3 != fscanf(fin, "%d %d %d\n", &xconv.ui, &yconv.ui, &color_in)) {
          break;
        } else {
          pos_in.x = xconv.fl;
          pos_in.y = yconv.fl;
          sfVertex tmpVx = {pos_in, sfColor_fromInteger(color_in)};
          g.vxa[nrVcs] = tmpVx;
          ++nrVcs;
        }
      }
      nrVcs2draw = nrVcs;
      preloaded = 1;
      fclose(fin);
    } else {
      fprintf(stderr, "Failed to load\n");
      cleanGarbage(g);
      return EXIT_FAILURE;
    }
  }
  if (argc > 2) {
    if (argv[1][0] == '-') {
      sfVideoMode vm = sfVideoMode_getDesktopMode();
      winsize.x = vm.width;
      winsize.y = vm.height;
    } else {
      winsize.x = strtoul(argv[1], 0, 10);
      winsize.y = strtoul(argv[2], 0, 10);
      if (errno == EINVAL || errno == ERANGE) {
        fprintf(stderr, "incorrect parameters :(\n");
        winsize.x = winsize.y = 1000;
        errno = 0;
      }
    }
  }

  sfVideoMode tmpVm = {winsize.x, winsize.y, 32};

  g.window =
      sfRenderWindow_create(tmpVm, "C Draw", sfClose | sfTitlebar, &settings);

  if (!g.window) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  g.crossyCursor = sfCursor_createFromSystem(sfCursorCross);

  if (!g.crossyCursor) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  sfRenderWindow_setMouseCursor(g.window, g.crossyCursor);

  g.vxb = sfVertexBuffer_create(nrMaxVecs, sfLines, sfVertexBufferStream);

  if (!g.vxb) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  if (preloaded && !sfVertexBuffer_update(g.vxb, g.vxa, nrVcs, 0)) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  centerVxs[0].color = tmpCol;
  centerVxs[1].color = tmpCol;
  centerVxs[2].color = tmpCol;
  centerVxs[3].color = tmpCol;

  sfVector2f tmpVec;
  tmpVec.x = winsize.x / 2 - 10;
  tmpVec.y = winsize.y / 2;
  centerVxs[0].position = tmpVec;
  tmpVec.x = winsize.x / 2 + 10;
  tmpVec.y = winsize.y / 2;
  centerVxs[1].position = tmpVec;
  tmpVec.x = winsize.x / 2;
  tmpVec.y = winsize.y / 2 - 10;
  centerVxs[2].position = tmpVec;
  tmpVec.x = winsize.x / 2;
  tmpVec.y = winsize.y / 2 + 10;
  centerVxs[3].position = tmpVec;

  sfVector2i oldMousePos;
  int drawing = 0;
  int nrVcsIncr = 0;
  int nrVcsDecr = 0;
  int nrVcsFineIncr = 0;
  int nrVcsFineDecr = 0;
  int zoomIncr = 0;
  int zoomDecr = 0;
  int rotateRight = 0;
  int rotateLeft = 0;
  int waitEvt = 1;

  int viewMoving = 0;
  int drawCross = 0;

  g.unredoClock = sfClock_create();
  g.zoomClock = sfClock_create();
  g.rotateClock = sfClock_create();

  if (!g.unredoClock || !g.zoomClock || !g.rotateClock) {
    cleanGarbage(g);
    return EXIT_FAILURE;
  }

  const size_t crcsz = 150;

  while (sfRenderWindow_isOpen(g.window)) {
    sfEvent evt;
    int enough2wait = 0;
    while (!enough2wait &&
           whateverEvent(waitEvt, g.window, &evt, &enough2wait)) {
      switch (evt.type) {
      case sfEvtMouseWheelScrolled: {
        sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
        if (!view) {
          cleanGarbage(g);
          return EXIT_FAILURE;
        }
        sfView_zoom(view, exp(evt.mouseWheelScroll.delta / 3.f));
        sfRenderWindow_setView(g.window, view);
        sfView_destroy(view);
        break;
      }
      case sfEvtMouseMoved: {
        sfVector2i mousePos = {evt.mouseMove.x, evt.mouseMove.y};
        sfVector2f oldMousePosGl = sfRenderWindow_mapPixelToCoords(
            g.window, oldMousePos, sfRenderWindow_getView(g.window));
        sfVector2f mousePosGl = sfRenderWindow_mapPixelToCoords(
            g.window, mousePos, sfRenderWindow_getView(g.window));
        if (viewMoving) {
          sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
          if (!view) {
            cleanGarbage(g);
            return EXIT_FAILURE;
          }
          sfVector2f tmpDelta;
          tmpDelta.x = oldMousePosGl.x - mousePosGl.x;
          tmpDelta.y = oldMousePosGl.y - mousePosGl.y;
          sfView_move(view, tmpDelta);
          sfRenderWindow_setView(g.window, view);
          oldMousePos = mousePos;
          sfView_destroy(view);
        } else if (!ruler && !circle && drawing && nrVcs2draw < nrMaxVecs - 2) {
          sfVertex vcs[2];
          vcs[0].position = oldMousePosGl;
          vcs[1].position = mousePosGl;
          vcs[0].color = vcs[1].color = color;
          if (sfVertexBuffer_update(g.vxb, vcs, 2, nrVcs)) {
            g.vxa[nrVcs + 0] = vcs[0];
            g.vxa[nrVcs + 1] = vcs[1];
            nrVcs += 2;
            nrVcs2draw += 2;
            oldMousePos = mousePos;
          }
        }
        break;
      }
      case sfEvtMouseButtonPressed:
        if ((evt.mouseButton.button == sfMouseLeft) &&
            ((nrVcs2draw < nrMaxVecs - 2 && !circle) ||
             (nrVcs2draw < nrMaxVecs - crcsz && circle)) &&
            !nrVcsDecr && !nrVcsIncr && !nrVcsFineDecr && !nrVcsFineIncr &&
            !zoomDecr && !zoomIncr && !rotateLeft && !rotateRight) {
          nrVcs = nrVcs2draw;
          drawing = 1;
        } else if (evt.mouseButton.button == sfMouseMiddle) {
          viewMoving = 1;
        }
        oldMousePos.x = evt.mouseButton.x;
        oldMousePos.y = evt.mouseButton.y;
        break;
      case sfEvtMouseButtonReleased:
        if ((evt.mouseButton.button == sfMouseLeft) && drawing) {
          drawing = 0;
          sfVector2i mousePos = {evt.mouseButton.x, evt.mouseButton.y};
          sfVector2f oldMousePosGl = sfRenderWindow_mapPixelToCoords(
              g.window, oldMousePos, sfRenderWindow_getView(g.window));
          sfVector2f mousePosGl = sfRenderWindow_mapPixelToCoords(
              g.window, mousePos, sfRenderWindow_getView(g.window));
          if (!circle) {
            sfVertex vcs[2];
            vcs[0].position = oldMousePosGl;
            vcs[1].position = mousePosGl;
            vcs[0].color = vcs[1].color = color;
            if (sfVertexBuffer_update(g.vxb, vcs, 2, nrVcs)) {
              g.vxa[nrVcs + 0] = vcs[0];
              g.vxa[nrVcs + 1] = vcs[1];
              nrVcs += 2;
              nrVcs2draw += 2;
            }

            if (nrVcs2draw > lastUpdatedNrVcs2draw) {
              struct UndoNode *newNode = malloc(sizeof(struct UndoNode));
              if (!newNode) {
                cleanGarbage(g);
                return EXIT_FAILURE;
              }
              newNode->prev = g.undos;
              newNode->data = nrVcs2draw - lastUpdatedNrVcs2draw;
              g.undos = newNode;
              lastUpdatedNrVcs2draw = nrVcs2draw;
            }

          } else {
            sfVertex circleVcs[crcsz];
            for (size_t i = 0; i < crcsz; ++i) {
              circleVcs[i].color = color;
            }
            float distance = hypot(mousePosGl.x - oldMousePosGl.x,
                                   mousePosGl.y - oldMousePosGl.y);
            for (size_t i = 0; i < crcsz / 2; ++i) {
              const float pi = 3.1415927f;

              circleVcs[i << 1].position.x =
                  oldMousePosGl.x + distance * cos(i * 4 * pi / crcsz);
              circleVcs[i << 1].position.y =
                  oldMousePosGl.y + distance * sin(i * 4 * pi / crcsz);

              circleVcs[i * 2 + 1].position.x =
                  oldMousePosGl.x + distance * cos(((i + 1) * 4 * pi / crcsz));
              circleVcs[i * 2 + 1].position.y =
                  oldMousePosGl.y + distance * sin(((i + 1) * 4 * pi / crcsz));
            }

            if (sfVertexBuffer_update(g.vxb, circleVcs, crcsz, nrVcs)) {
              for (size_t i = 0; i < crcsz; ++i) {
                g.vxa[nrVcs + i] = circleVcs[i];
              }
              nrVcs += crcsz;
              nrVcs2draw += crcsz;
            }

            if (nrVcs2draw > lastUpdatedNrVcs2draw) {
              struct UndoNode *newNode = malloc(sizeof(struct UndoNode));
              if (!newNode) {
                cleanGarbage(g);
                return EXIT_FAILURE;
              }
              newNode->prev = g.undos;
              newNode->data = nrVcs2draw - lastUpdatedNrVcs2draw;
              g.undos = newNode;
              lastUpdatedNrVcs2draw = nrVcs2draw;
            }
          }
          waitEvt = 1;
        } else if (evt.mouseButton.button == sfMouseMiddle) {
          viewMoving = 0;
        }
        break;
      case sfEvtClosed: {
        sfRenderWindow_close(g.window);

        if (argc > 3) {
          save_to(argv[3], g.vxa, nrVcs, winsize);
        } else {
          save(g.vxa, nrVcs, winsize);
        }
        break;
      }
      case sfEvtLostFocus:
        waitEvt = 1;
        nrVcsIncr = 0;
        nrVcsDecr = 0;
        viewMoving = 0;
        nrVcsFineDecr = 0;
        nrVcsFineIncr = 0;
        zoomDecr = 0;
        zoomIncr = 0;
        rotateLeft = 0;
        rotateRight = 0;
        if (drawing) {
          drawing = 0;
          sfVector2i mousePos = {evt.mouseButton.x, evt.mouseButton.y};
          sfVector2f oldMousePosGl = sfRenderWindow_mapPixelToCoords(
              g.window, oldMousePos, sfRenderWindow_getView(g.window));
          sfVector2f mousePosGl = sfRenderWindow_mapPixelToCoords(
              g.window, mousePos, sfRenderWindow_getView(g.window));

          sfVertex vcs[2];
          vcs[0].position = oldMousePosGl;
          vcs[1].position = mousePosGl;
          vcs[0].color = vcs[1].color = color;
          if (sfVertexBuffer_update(g.vxb, vcs, 2, nrVcs)) {
            g.vxa[nrVcs + 0] = vcs[0];
            g.vxa[nrVcs + 1] = vcs[1];
            nrVcs += 2;
            nrVcs2draw += 2;
          }
        }
        break;
      case sfEvtKeyPressed:
        if (!drawing) {
          if (evt.key.code == sfKeyLeft) {
            nrVcsDecr = 1;
            waitEvt = 0;
            sfClock_restart(g.unredoClock);
          } else if (evt.key.code == sfKeyRight) {
            nrVcsIncr = 1;
            waitEvt = 0;
            sfClock_restart(g.unredoClock);
          } else if (evt.key.code == sfKeyUp) {
            nrVcsFineIncr = 1;
            waitEvt = 0;
            sfClock_restart(g.unredoClock);
          } else if (evt.key.code == sfKeyDown) {
            nrVcsFineDecr = 1;
            waitEvt = 0;
            sfClock_restart(g.unredoClock);
          } else if (evt.key.code == sfKeyS) {
            if (evt.key.control) {
              save(g.vxa, nrVcs, winsize);
            }
          } else if (evt.key.code == sfKeyE) {
            if (evt.key.control) {
              exportSvg(g.vxa, nrVcs);
            }
          } else if (evt.key.code == sfKeyW) {
            ruler = !ruler;
            circle = 0;
          } else if (evt.key.code == sfKeyC) {
            circle = !circle;
            ruler = 0;
          } else if (evt.key.code == sfKeyR) {
            if (evt.key.alt) {
              nrVcs2draw = 0;
            }
          } else if (evt.key.code == sfKeyF) {
            if (evt.key.alt) {
              nrVcs2draw = nrVcs;
            } else {
              sfRenderWindow_setView(g.window,
                                     sfRenderWindow_getDefaultView(g.window));
            }
          } else if (evt.key.code == sfKeyZ) {
            if (evt.key.control) {
              if (g.undos) {
                if (nrVcs2draw >= g.undos->data) {
                  nrVcs2draw -= g.undos->data;
                  struct UndoNode *node2delete = g.undos;
                  g.undos = g.undos->prev;
                  free(node2delete);
                }
              }
            } else {
              zoomDecr = 1;
              waitEvt = 0;
              sfClock_restart(g.zoomClock);
            }
          } else if (evt.key.code == sfKeyX) {
            zoomIncr = 1;
            waitEvt = 0;
            sfClock_restart(g.zoomClock);
          } else if (evt.key.code == sfKeySlash) {
            rotateLeft = 1;
            waitEvt = 0;
            sfClock_restart(g.rotateClock);
          } else if (evt.key.code == sfKeyPeriod) {
            rotateRight = 1;
            waitEvt = 0;
            sfClock_restart(g.rotateClock);
          } else if (evt.key.code == sfKeyB) {
            if (evt.key.control) {
              drawCross = !drawCross;
            } else {
              sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
              if (!view) {
                cleanGarbage(g);
                return EXIT_FAILURE;
              }
              sfView_setSize(view, (sfVector2f){winsize.x, winsize.y});
              sfRenderWindow_setView(g.window, view);
              sfView_destroy(view);
            }
          }
        }

        if (evt.key.code == sfKeyNum0) {
          color = (sfColor){18, 20, 31, 255};
        } else if (evt.key.code == sfKeyNum1) {
          color = (sfColor){233, 243, 255, 255};
        } else if (evt.key.code == sfKeyNum2) {
          color = (sfColor){127, 216, 240, 255};
        } else if (evt.key.code == sfKeyNum3) {
          color = (sfColor){228, 64, 168, 255};
        } else if (evt.key.code == sfKeyNum4) {
          color = (sfColor){231, 228, 180, 255};
        } else if (evt.key.code == sfKeyNum5) {
          color = (sfColor){115, 124, 242, 255};
        } else if (evt.key.code == sfKeyNum6) {
          color = (sfColor){126, 218, 177, 255};
        } else if (evt.key.code == sfKeyNum7) {
          color = (sfColor){206, 173, 243, 255};
        } else if (evt.key.code == sfKeyNum8) {
          color = (sfColor){130, 134, 133, 255};
        } else if (evt.key.code == sfKeyNum9) {
          color = (sfColor){255, 0, 0, 255};
        }
        break;
      case sfEvtKeyReleased:
        if (evt.key.code == sfKeyLeft || evt.key.code == sfKeyRight ||
            evt.key.code == sfKeyUp || evt.key.code == sfKeyDown) {
          nrVcsDecr = nrVcsIncr = 0;
          nrVcsFineDecr = nrVcsFineIncr = 0;
          waitEvt = 1;
          if (nrVcs2draw > lastUpdatedNrVcs2draw) {
            struct UndoNode *newNode = malloc(sizeof(struct UndoNode));
            if (!newNode) {
              cleanGarbage(g);
              return EXIT_FAILURE;
            }
            newNode->prev = g.undos;
            newNode->data = nrVcs2draw - lastUpdatedNrVcs2draw;
            g.undos = newNode;
            lastUpdatedNrVcs2draw = nrVcs2draw;
          }
        } else if (evt.key.code == sfKeyZ || evt.key.code == sfKeyX ||
                   evt.key.code == sfKeyPeriod || evt.key.code == sfKeySlash) {
          zoomDecr = 0;
          zoomIncr = 0;
          rotateLeft = 0;
          rotateRight = 0;
          waitEvt = 1;
        }
        break;
      default:
        break;
      }
    }

    if (nrVcsDecr) {
      size_t delta = sfClock_restart(g.unredoClock).microseconds;
      delta *= 2;
      if (delta >= nrVcs2draw) {
        nrVcs2draw = 0;
      } else {
        nrVcs2draw -= delta;
      }
    } else if (nrVcsIncr) {
      size_t delta = sfClock_restart(g.unredoClock).microseconds;
      delta *= 2;
      if (nrVcs <= nrVcs2draw + delta) {
        nrVcs2draw = nrVcs;
      } else {
        nrVcs2draw += delta;
      }
    } else if (nrVcsFineDecr) {
      sfClock_restart(g.unredoClock);
      size_t delta = 2;
      if (delta >= nrVcs2draw) {
        nrVcs2draw = 0;
      } else {
        nrVcs2draw -= delta;
      }
    } else if (nrVcsFineIncr) {
      sfClock_restart(g.unredoClock);
      size_t delta = 2;
      if (nrVcs <= nrVcs2draw + delta) {
        nrVcs2draw = nrVcs;
      } else {
        nrVcs2draw += delta;
      }
    }

    if (zoomIncr) {
      sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
      if (!view) {
        cleanGarbage(g);
        return EXIT_FAILURE;
      }

      sfTime time = sfClock_restart(g.zoomClock);
      sfView_zoom(view, exp(time.microseconds / 1000000.f));
      sfRenderWindow_setView(g.window, view);
      sfView_destroy(view);
    } else if (zoomDecr) {
      sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
      if (!view) {
        cleanGarbage(g);
        return EXIT_FAILURE;
      }
      sfTime time = sfClock_restart(g.zoomClock);
      sfView_zoom(view, exp(-time.microseconds / 1000000.f));
      sfRenderWindow_setView(g.window, view);
      sfView_destroy(view);
    }

    if (rotateLeft) {
      sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
      if (!view) {
        cleanGarbage(g);
        return EXIT_FAILURE;
      }
      sfTime time = sfClock_restart(g.rotateClock);
      sfView_rotate(view, time.microseconds / 1000000.f * 80);
      sfRenderWindow_setView(g.window, view);
      sfView_destroy(view);
    } else if (rotateRight) {
      sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
      if (!view) {
        cleanGarbage(g);
        return EXIT_FAILURE;
      }

      sfTime time = sfClock_restart(g.rotateClock);
      sfView_rotate(view, -time.microseconds / 1000000.f * 80);
      sfRenderWindow_setView(g.window, view);
      sfView_destroy(view);
    }

    sfRenderWindow_clear(g.window, (sfColor){18, 20, 31, 255});
    sfView *view = sfView_copy(sfRenderWindow_getView(g.window));
    if (!view) {
      cleanGarbage(g);
      return EXIT_FAILURE;
    }
    sfRenderWindow_drawVertexBufferRange(g.window, g.vxb, 0, nrVcs2draw, NULL);

    if (drawCross) {
      sfRenderWindow_setView(g.window, sfRenderWindow_getDefaultView(g.window));
      sfRenderWindow_drawPrimitives(g.window, centerVxs, 4, sfLines, NULL);
      sfRenderWindow_setView(g.window, view);
    }
    sfRenderWindow_display(g.window);
    sfView_destroy(view);
  }

  cleanGarbage(g);
  return EXIT_SUCCESS;
}