#ifndef RADIO_DEBUG_H
#define RADIO_DEBUG_H

#ifdef RADIO_DEBUG
#define RADIO_DBG(...) do { __VA_ARGS__; } while (0);
#define RADIO_DBG_PRINTF(x) do { printf x; } while (0);
#else
#define RADIO_DBG(...) do { } while (0);
#define RADIO_DBG_PRINTF(x) do { } while (0);
#endif

#endif /* RADIO_DEBUG_H */
