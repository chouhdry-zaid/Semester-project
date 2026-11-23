/*
 * sanjay.c
 * FAST Lost & Found Management System
 * Language: C | GUI: Raylib
 *
 * Part 2 of the project (Sanjay Kumar).
 * Everything that is not drawing: data storage, loading / saving the text
 * files in ./data/, the matching score, the report form checks and the
 * verification / return steps. The GUI in main.c calls these functions.
 */

#include "sanjay.h"

/* main.c defines this and includes this file, so only main.c has to be compiled */
#ifdef LF_BUILD_WITH_MAIN

const char *categories[] = {
    "Keys","Wallet","Student ID Card","Mobile Phone","Laptop","Books",
    "Water Bottle","Bag","Earphones","Watch","Other"
};

FoundItem foundItems[MAX_ITEMS];
LostItem lostItems[MAX_ITEMS];
Message messages[MAX_MESSAGES];
int foundCount = 0, lostCount = 0, messageCount = 0;
int nextFoundId = 1001, nextLostId = 2001, nextMessageId = 1;
Student currentUser = {"Demo Student A", "FAST-001", "03000000000"};

char notice[200] = "";
float noticeTimer = 0;

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

void trimNewline(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';
    }
}

void setNotice(const char *s) {
    snprintf(notice, sizeof(notice), "%s", s);
    noticeTimer = 4.0f;
}

int containsIgnoreCase(const char *hay, const char *needle) {
    if (!needle[0]) return 1;

    char a[512], b[256];
    snprintf(a, sizeof(a), "%s", hay);
    snprintf(b, sizeof(b), "%s", needle);

    for (int i = 0; a[i]; i++) a[i] = (char)tolower((unsigned char)a[i]);
    for (int i = 0; b[i]; i++) b[i] = (char)tolower((unsigned char)b[i]);

    return strstr(a, b) != NULL;
}

int sameText(const char *a, const char *b) {
    return containsIgnoreCase(a, b) && containsIgnoreCase(b, a);
}

int validContact(const char *s) {
    int digits = 0;
    for (int i = 0; s[i]; i++) {
        if (isdigit((unsigned char)s[i])) digits++;
    }
    return digits >= 7 && digits <= 15;
}

int statusIsActive(int s) {
    return s != ST_RETURNED && s != ST_CLOSED;
}

const char *statusName(int s) {
    switch (s) {
        case ST_AVAILABLE:     return "Available";
        case ST_VERIFY_PENDING:return "Verification pending";
        case ST_VERIFY_FAILED: return "Verification failed";
        case ST_VERIFIED:      return "Verified";
        case ST_RETURN_PENDING:return "Return pending";
        case ST_RETURNED:      return "Returned";
        default:               return "Closed";
    }
}

/* ------------------------------------------------------------------ */
/* File handling                                                       */
/* ------------------------------------------------------------------ */

void ensureDataFiles(void) {
#ifdef _WIN32
    system("if not exist data mkdir data");
#else
    system("mkdir -p data");
#endif
    /* opening in append mode creates the file if it is missing */
    FILE *f = fopen(LOST_FILE, "a");
    if (f) fclose(f);
    f = fopen(FOUND_FILE, "a");
    if (f) fclose(f);
    f = fopen(MSG_FILE, "a");
    if (f) fclose(f);
    f = fopen(STUDENT_FILE, "a");
    if (f) fclose(f);
}

void saveFound(void) {
    FILE *f = fopen(FOUND_FILE, "w");
    if (!f) {
        setNotice("Could not save found items.");
        return;
    }
    for (int i = 0; i < foundCount; i++) {
        FoundItem *x = &foundItems[i];
        fprintf(f, "%d|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d\n",
            x->id, x->category, x->name, x->description, x->location, x->building,
            x->room, x->date, x->colour, x->features, x->owner, x->studentId,
            x->contact, x->status, x->claimedBy);
    }
    fclose(f);
}

void saveLost(void) {
    FILE *f = fopen(LOST_FILE, "w");
    if (!f) {
        setNotice("Could not save lost items.");
        return;
    }
    for (int i = 0; i < lostCount; i++) {
        LostItem *x = &lostItems[i];
        fprintf(f, "%d|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d\n",
            x->id, x->category, x->name, x->description, x->location, x->building,
            x->room, x->date, x->colour, x->features, x->owner, x->studentId,
            x->contact, x->status, x->matchedFoundId);
    }
    fclose(f);
}

void saveMessages(void) {
    FILE *f = fopen(MSG_FILE, "w");
    if (!f) {
        setNotice("Could not save messages.");
        return;
    }
    for (int i = 0; i < messageCount; i++) {
        Message *m = &messages[i];
        fprintf(f, "%d|%d|%d|%s|%s|%s|%d\n", m->id, m->foundId, m->lostId,
                m->sender, m->kind, m->text, m->timestamp);
    }
    fclose(f);
}

void loadData(void) {
    char line[1800];

    /* found items */
    FILE *f = fopen(FOUND_FILE, "r");
    if (f) {
        while (fgets(line, sizeof(line), f) && foundCount < MAX_ITEMS) {
            FoundItem x = {0};
            int n = sscanf(line, "%d|%d|%159[^|]|%255[^|]|%159[^|]|%159[^|]|%159[^|]|%159[^|]|%159[^|]|%255[^|]|%159[^|]|%159[^|]|%31[^|]|%d|%d",
                &x.id, &x.category, x.name, x.description, x.location, x.building, x.room,
                x.date, x.colour, x.features, x.owner, x.studentId, x.contact, &x.status, &x.claimedBy);
            if (n >= 14) {
                foundItems[foundCount++] = x;
                if (x.id >= nextFoundId) nextFoundId = x.id + 1;
            }
        }
        fclose(f);
    }

    /* lost items */
    f = fopen(LOST_FILE, "r");
    if (f) {
        while (fgets(line, sizeof(line), f) && lostCount < MAX_ITEMS) {
            LostItem x = {0};
            int n = sscanf(line, "%d|%d|%159[^|]|%255[^|]|%159[^|]|%159[^|]|%159[^|]|%159[^|]|%159[^|]|%255[^|]|%159[^|]|%159[^|]|%31[^|]|%d|%d",
                &x.id, &x.category, x.name, x.description, x.location, x.building, x.room,
                x.date, x.colour, x.features, x.owner, x.studentId, x.contact, &x.status, &x.matchedFoundId);
            if (n >= 14) {
                lostItems[lostCount++] = x;
                if (x.id >= nextLostId) nextLostId = x.id + 1;
            }
        }
        fclose(f);
    }

    /* chat messages */
    f = fopen(MSG_FILE, "r");
    if (f) {
        while (fgets(line, sizeof(line), f) && messageCount < MAX_MESSAGES) {
            Message m = {0};
            int n = sscanf(line, "%d|%d|%d|%159[^|]|%23[^|]|%255[^|]|%d",
                &m.id, &m.foundId, &m.lostId, m.sender, m.kind, m.text, &m.timestamp);
            if (n == 7) {
                messages[messageCount++] = m;
                if (m.id >= nextMessageId) nextMessageId = m.id + 1;
            }
        }
        fclose(f);
    }
}

/* ------------------------------------------------------------------ */
/* Messages, lookup and matching                                       */
/* ------------------------------------------------------------------ */

void addMessage(int foundId, int lostId, const char *sender, const char *kind, const char *text) {
    if (messageCount >= MAX_MESSAGES) {
        setNotice("Message storage is full.");
        return;
    }
    Message *m = &messages[messageCount++];
    memset(m, 0, sizeof(*m));
    m->id = nextMessageId++;
    m->foundId = foundId;
    m->lostId = lostId;
    snprintf(m->sender, sizeof(m->sender), "%s", sender);
    snprintf(m->kind, sizeof(m->kind), "%s", kind);
    snprintf(m->text, sizeof(m->text), "%s", text);
    m->timestamp = (int)time(NULL);
    saveMessages();
}

int findFoundIndex(int id) {
    for (int i = 0; i < foundCount; i++) {
        if (foundItems[i].id == id) return i;
    }
    return -1;
}

int findLostIndex(int id) {
    for (int i = 0; i < lostCount; i++) {
        if (lostItems[i].id == id) return i;
    }
    return -1;
}

int matchScore(const LostItem *l, const FoundItem *f) {
    /* different category or an item that is already gone = no match */
    if (l->category != f->category || !statusIsActive(f->status)) return 0;

    int score = 35;

    if (sameText(l->name, f->name)) score += 25;
    else if (containsIgnoreCase(l->name, f->name) || containsIgnoreCase(f->name, l->name)) score += 12;

    if (l->location[0] && f->location[0] && sameText(l->location, f->location)) score += 15;
    else if (l->building[0] && f->building[0] && sameText(l->building, f->building)) score += 8;

    if (l->date[0] && f->date[0] && sameText(l->date, f->date)) score += 10;
    if (l->colour[0] && f->colour[0] && sameText(l->colour, f->colour)) score += 10;

    if (l->description[0] && f->description[0] &&
        (containsIgnoreCase(l->description, f->description) ||
         containsIgnoreCase(f->description, l->description))) score += 5;

    if (score > 100) score = 100;
    return score;
}

/* ------------------------------------------------------------------ */
/* Report form                                                         */
/* ------------------------------------------------------------------ */

void resetForm(int isLost) {
    memset(form, 0, sizeof(form));
    formCategory = 0;
    formIsLost = isLost;
    formFocus = -1;

    /* pre-fill the owner details from the selected profile */
    snprintf(form[8], MAX_DESC, "%s", currentUser.name);
    snprintf(form[9], MAX_DESC, "%s", currentUser.studentId);
    snprintf(form[10], MAX_DESC, "%s", currentUser.contact);
}

int formValid(void) {
    if (!form[0][0] || !form[2][0] || !form[8][0] || !form[9][0] || !validContact(form[10])) {
        return 0;
    }
    return 1;
}

void submitForm(void) {
    if (!formValid()) {
        setNotice("Complete required fields and enter a valid contact number.");
        return;
    }

    if (formIsLost) {
        if (lostCount >= MAX_ITEMS) {
            setNotice("Lost-item storage is full.");
            return;
        }
        LostItem *x = &lostItems[lostCount++];
        memset(x, 0, sizeof(*x));
        x->id = nextLostId++;
        x->category = formCategory;
        snprintf(x->name, sizeof(x->name), "%s", form[0]);
        snprintf(x->description, sizeof(x->description), "%s", form[1]);
        snprintf(x->location, sizeof(x->location), "%s", form[2]);
        snprintf(x->building, sizeof(x->building), "%s", form[3]);
        snprintf(x->room, sizeof(x->room), "%s", form[4]);
        snprintf(x->date, sizeof(x->date), "%s", form[5]);
        snprintf(x->colour, sizeof(x->colour), "%s", form[6]);
        snprintf(x->features, sizeof(x->features), "%s", form[7]);
        snprintf(x->owner, sizeof(x->owner), "%s", form[8]);
        snprintf(x->studentId, sizeof(x->studentId), "%s", form[9]);
        snprintf(x->contact, sizeof(x->contact), "%s", form[10]);
        x->status = ST_AVAILABLE;
        x->matchedFoundId = -1;
        saveLost();

        selectedLost = lostCount - 1;
        page = PAGE_MATCHES;
        setNotice("Lost report saved. Review possible matches.");
    } else {
        if (foundCount >= MAX_ITEMS) {
            setNotice("Found-item storage is full.");
            return;
        }
        FoundItem *x = &foundItems[foundCount++];
        memset(x, 0, sizeof(*x));
        x->id = nextFoundId++;
        x->category = formCategory;
        snprintf(x->name, sizeof(x->name), "%s", form[0]);
        snprintf(x->description, sizeof(x->description), "%s", form[1]);
        snprintf(x->location, sizeof(x->location), "%s", form[2]);
        snprintf(x->building, sizeof(x->building), "%s", form[3]);
        snprintf(x->room, sizeof(x->room), "%s", form[4]);
        snprintf(x->date, sizeof(x->date), "%s", form[5]);
        snprintf(x->colour, sizeof(x->colour), "%s", form[6]);
        snprintf(x->features, sizeof(x->features), "%s", form[7]);
        snprintf(x->owner, sizeof(x->owner), "%s", form[8]);
        snprintf(x->studentId, sizeof(x->studentId), "%s", form[9]);
        snprintf(x->contact, sizeof(x->contact), "%s", form[10]);
        x->status = ST_AVAILABLE;
        x->claimedBy = -1;
        saveFound();

        page = PAGE_HOME;
        setNotice("Found item registered successfully.");
    }
}

/* ------------------------------------------------------------------ */
/* Verification and return workflow                                    */
/* ------------------------------------------------------------------ */

int isOwnedByCurrent(const char *studentId) {
    return strcmp(studentId, currentUser.studentId) == 0;
}

void openChat(int fi, int li) {
    selectedChatFound = fi;
    selectedChatLost = li;
    page = PAGE_CHAT;
}

void sendVerificationRequest(int fi, int li) {
    if (fi < 0 || li < 0) return;

    FoundItem *f = &foundItems[fi];
    LostItem *l = &lostItems[li];

    if (!statusIsActive(f->status)) {
        setNotice("This item is no longer available.");
        return;
    }

    f->status = ST_VERIFY_PENDING;
    f->claimedBy = l->id;
    l->matchedFoundId = f->id;
    l->status = ST_VERIFY_PENDING;
    saveFound();
    saveLost();

    addMessage(f->id, l->id, l->owner, "REQUEST", "A student has requested verification for this item.");
    openChat(fi, li);
    setNotice("Verification request created.");
}

void markReturned(void) {
    if (selectedChatFound < 0 || selectedChatLost < 0) return;

    FoundItem *f = &foundItems[selectedChatFound];
    LostItem *l = &lostItems[selectedChatLost];

    f->status = ST_RETURNED;
    l->status = ST_RETURNED;
    saveFound();
    saveLost();
    addMessage(f->id, l->id, currentUser.name, "SYSTEM", "Item marked as returned. Thank you!");

    confirmReturn = false;
    setNotice("Item marked as returned. It is removed from active matches.");
}

void approveVerification(int ok) {
    if (selectedChatFound < 0 || selectedChatLost < 0) return;

    FoundItem *f = &foundItems[selectedChatFound];
    LostItem *l = &lostItems[selectedChatLost];

    if (ok) {
        f->status = ST_VERIFIED;
        l->status = ST_VERIFIED;
        addMessage(f->id, l->id, currentUser.name, "SYSTEM", "Finder confirmed the claimant's answers.");
        setNotice("Verification confirmed.");
    } else {
        f->status = ST_AVAILABLE;
        f->claimedBy = -1;
        l->status = ST_VERIFY_FAILED;
        l->matchedFoundId = -1;
        addMessage(f->id, l->id, currentUser.name, "SYSTEM", "Verification was not successful. Item remains available.");
        setNotice("Verification failed. Item remains available.");
    }
    saveFound();
    saveLost();
}

void startReturn(void) {
    if (selectedChatFound < 0 || selectedChatLost < 0) return;

    foundItems[selectedChatFound].status = ST_RETURN_PENDING;
    lostItems[selectedChatLost].status = ST_RETURN_PENDING;
    saveFound();
    saveLost();
    setNotice("Return handover marked as pending.");
}

#endif /* LF_BUILD_WITH_MAIN */
