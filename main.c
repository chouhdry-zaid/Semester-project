/*
 * FAST Lost & Found Management System
 * Language: C | GUI: Raylib
 *
 * Local desktop prototype. Multiple demo student profiles can be selected
 * from the top-right profile control. Data is persisted in ./data/.
 *
 * Part 1 of the project: all the screens (home, forms, browse, matches,
 * reports, chat) and the main loop. The data and workflow side lives in
 * sanjay.c / sanjay.h (Sanjay Kumar).
 *
 * Build (Linux):
 *   gcc main.c -o fast_lost_found -lraylib -lm -lpthread -ldl -lrt -lX11
 *
 * Build (Windows / MinGW, if Raylib is installed):
 *   gcc main.c -o fast_lost_found.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 */

#include "raylib.h"
#include "sanjay.h"

/*
 * Notepad++ / raylib only compiles main.c, so main.c pulls in Sanjay's
 * code itself. The macro below tells sanjay.c that it is being built as
 * part of main.c. (If sanjay.c is also compiled on its own it simply
 * produces nothing, so there are no "multiple definition" errors.)
 */
#define LF_BUILD_WITH_MAIN
#include "sanjay.c"

/* Screen state (sanjay.c uses these through the extern lines in sanjay.h). */
Page page = PAGE_HOME;
int selectedLost = -1, selectedFound = -1;
int selectedChatFound = -1, selectedChatLost = -1;
int confirmReturn = false;

static const char *demoNames[] = {"Demo Student A", "Demo Student B", "Demo Student C"};
static const char *demoIds[] = {"FAST-001", "FAST-002", "FAST-003"};
static const char *demoContacts[] = {"03000000000", "03111111111", "03222222222"};
static int currentProfile = 0;

static char searchText[100] = "";
static int searchEdit = false;
static int categoryFilter = -1;
static int scrollOffset = 0;

/* Form fields. */
char form[FORM_FIELDS][MAX_DESC];
int formFocus = -1;
int formCategory = 0;
int formIsLost = 0;
static const char *formLabels[] = {
    "Item name","Description","Location (e.g. Library)","Building / department",
    "Room / classroom","Date (YYYY-MM-DD)","Colour","Distinguishing features",
    "Your name","FAST student ID","Contact number"
};

/* ------------------------------------------------------------------ */
/* Drawing helpers                                                     */
/* ------------------------------------------------------------------ */

static void drawTextFit(const char *s, int x, int y, int size, Color c, int maxWidth) {
    DrawText(s, x, y, size, c);
    (void)maxWidth;
}

static int pointIn(Rectangle r) {
    Vector2 m = GetMousePosition();
    return CheckCollisionPointRec(m, r);
}

static int button(const char *label, Rectangle r, Color bg, Color fg) {
    int hover = pointIn(r);
    DrawRectangleRounded(r, 0.18f, 8, hover ? ColorBrightness(bg, 0.08f) : bg);
    DrawRectangleRoundedLines(r, 0.18f, 8, C_BORDER);

    int fs = 18, tw = MeasureText(label, fs);
    DrawText(label, (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height/2 - fs/2), fs, fg);

    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void panel(Rectangle r) {
    DrawRectangleRounded(r, 0.04f, 8, C_PANEL);
    DrawRectangleRoundedLines(r, 0.04f, 8, C_BORDER);
}

static void label(const char *s, int x, int y, int size, Color c) {
    DrawText(s, x, y, size, c);
}

static void header(const char *subtitle) {
    DrawRectangle(0, 0, W, 88, C_NAVY);
    DrawText("FAST", 34, 18, 30, RAYWHITE);
    DrawText("LOST & FOUND", 122, 24, 23, (Color){190,211,245,255});
    DrawText(subtitle, 36, 59, 13, (Color){190,201,219,255});

    DrawRectangleRounded((Rectangle){930,22,215,44}, 0.25f, 8, (Color){48,65,96,255});
    DrawText(TextFormat("Profile: %s", currentUser.name), 945, 36, 14, RAYWHITE);

    /* clicking the profile box cycles through the three demo students */
    if (pointIn((Rectangle){930,22,215,44}) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        currentProfile = (currentProfile + 1) % 3;
        snprintf(currentUser.name, sizeof(currentUser.name), "%s", demoNames[currentProfile]);
        snprintf(currentUser.studentId, sizeof(currentUser.studentId), "%s", demoIds[currentProfile]);
        snprintf(currentUser.contact, sizeof(currentUser.contact), "%s", demoContacts[currentProfile]);
        setNotice(TextFormat("Switched to %s (demo profile).", currentUser.name));
    }
}

static void drawNotice(void) {
    if (noticeTimer > 0) {
        Color bg = C_GREENBG, fg = C_GREEN;
        if (strstr(notice, "Could not") || strstr(notice, "valid") || strstr(notice, "full")) {
            bg = C_REDBG;
            fg = C_RED;
        }
        DrawRectangleRounded((Rectangle){300,700,580,42}, 0.2f, 8, bg);
        int tw = MeasureText(notice, 15);
        DrawText(notice, 590 - tw/2, 713, 15, fg);
    }
}

/* ------------------------------------------------------------------ */
/* Screens                                                             */
/* ------------------------------------------------------------------ */

static void drawHome(void) {
    header("A campus community space to reunite people with their belongings");
    DrawText("Welcome back,", 38, 121, 20, C_MUTED);
    DrawText(currentUser.name, 38, 148, 31, C_NAVY);
    DrawText("What would you like to do today?", 38, 198, 17, C_TEXT);

    Rectangle a = {38,244,340,145}, b = {420,244,340,145}, c = {802,244,340,145};
    panel(a);
    panel(b);
    panel(c);

    DrawCircle(82, 288, 20, C_BLUE2);
    DrawText("+", 73, 270, 35, C_BLUE);
    DrawText("Report a lost item", 116, 270, 21, C_NAVY);
    DrawText("Tell us what you have misplaced.", 116, 305, 14, C_MUTED);
    DrawText("Find possible matches.", 116, 329, 14, C_MUTED);

    DrawCircle(464, 288, 20, C_GREENBG);
    DrawText("+", 455, 270, 35, C_GREEN);
    DrawText("Report a found item", 498, 270, 21, C_NAVY);
    DrawText("Register something found", 498, 305, 14, C_MUTED);
    DrawText("and help return it.", 498, 329, 14, C_MUTED);

    DrawCircle(846, 288, 20, C_AMBERBG);
    DrawText("?", 838, 273, 27, C_AMBER);
    DrawText("Browse found items", 880, 270, 21, C_NAVY);
    DrawText("Explore active campus", 880, 305, 14, C_MUTED);
    DrawText("found-item reports.", 880, 329, 14, C_MUTED);

    DrawText("Quick overview", 38, 430, 22, C_NAVY);
    panel((Rectangle){38,472,1104,150});

    int active = 0, returned = 0, pending = 0;
    for (int i = 0; i < foundCount; i++) {
        if (foundItems[i].status == ST_RETURNED) returned++;
        else if (foundItems[i].status == ST_VERIFY_PENDING) pending++;
        else if (statusIsActive(foundItems[i].status)) active++;
    }
    DrawText(TextFormat("%d", active), 78, 500, 35, C_BLUE);
    DrawText("Active found items", 78, 545, 15, C_MUTED);
    DrawText(TextFormat("%d", lostCount), 380, 500, 35, C_AMBER);
    DrawText("Lost reports", 380, 545, 15, C_MUTED);
    DrawText(TextFormat("%d", pending), 680, 500, 35, C_GREEN);
    DrawText("Verification pending", 680, 545, 15, C_MUTED);
    DrawText(TextFormat("%d", returned), 950, 500, 35, C_NAVY);
    DrawText("Returned", 950, 545, 15, C_MUTED);

    if (button("REPORT LOST", (Rectangle){38,650,210,42}, C_BLUE, RAYWHITE)) {
        resetForm(1);
        page = PAGE_LOST_FORM;
    }
    if (button("REPORT FOUND", (Rectangle){266,650,210,42}, C_GREEN, RAYWHITE)) {
        resetForm(0);
        page = PAGE_FOUND_FORM;
    }
    if (button("BROWSE ITEMS", (Rectangle){494,650,210,42}, C_PANEL, C_TEXT)) page = PAGE_BROWSE;
    if (button("MY REPORTS", (Rectangle){722,650,210,42}, C_PANEL, C_TEXT)) page = PAGE_REPORTS;
    if (button("EXIT", (Rectangle){950,650,192,42}, C_REDBG, C_RED)) CloseWindow();
}

static void drawForm(void) {
    int isLost = (page == PAGE_LOST_FORM);
    header(isLost ? "Report an item you have lost" : "Register an item you have found");
    DrawText(isLost ? "Lost item report" : "Found item registration", 38, 112, 27, C_NAVY);
    DrawText("Fields marked * are required. Contact details are kept private in this local prototype.", 38, 148, 14, C_MUTED);

    /* category box, click to go to the next one */
    DrawText("Item category *", 38, 190, 15, C_TEXT);
    Rectangle cat = {38,215,300,42};
    DrawRectangleRounded(cat, 0.15f, 8, C_PANEL);
    DrawRectangleRoundedLines(cat, 0.15f, 8, C_BORDER);
    DrawText(categories[formCategory], 52, 227, 17, C_BLUE);
    if (pointIn(cat) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        formCategory = (formCategory + 1) % CATEGORY_COUNT;
    }
    DrawText("(click to change category)", 350, 228, 13, C_MUTED);

    /* the text fields, two per row */
    int y0 = 275;
    for (int i = 0; i < 11; i++) {
        int col = i % 2, row = i / 2;
        int x = 38 + col*555, y = y0 + row*66;

        DrawText(formLabels[i], x, y, 14, C_TEXT);
        Rectangle r = {(float)x, (float)y + 21, 515, 36};
        DrawRectangleRounded(r, 0.12f, 6, formFocus == i ? C_BLUE2 : C_PANEL);
        DrawRectangleRoundedLines(r, 0.12f, 6, formFocus == i ? C_BLUE : C_BORDER);
        DrawText(form[i][0] ? form[i] : "Enter here...", x + 12, y + 30, 15, form[i][0] ? C_TEXT : C_MUTED);
        if (pointIn(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) formFocus = i;
    }

    /* typing into the focused field */
    if (formFocus >= 0) {
        int ch = GetCharPressed();
        while (ch > 0) {
            int len = (int)strlen(form[formFocus]);
            if (ch >= 32 && ch <= 125 && len < MAX_DESC - 2) {
                form[formFocus][len] = (char)ch;
                form[formFocus][len+1] = '\0';
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int n = (int)strlen(form[formFocus]);
            if (n > 0) form[formFocus][n-1] = '\0';
        }
        if (IsKeyPressed(KEY_TAB)) formFocus = (formFocus + 1) % 11;
    }

    if (button("SAVE REPORT", (Rectangle){38,672,190,42}, C_BLUE, RAYWHITE)) submitForm();
    if (button("CANCEL", (Rectangle){244,672,150,42}, C_PANEL, C_TEXT)) {
        page = PAGE_HOME;
        formFocus = -1;
    }
}

static void drawBrowse(void) {
    header("Browse active found-item reports");
    DrawText("Browse found items", 38, 112, 27, C_NAVY);

    /* search box */
    Rectangle sr = {38,157,390,42};
    DrawRectangleRounded(sr, 0.15f, 8, C_PANEL);
    DrawRectangleRoundedLines(sr, 0.15f, 8, C_BORDER);
    DrawText(searchText[0] ? searchText : "Search item name or location...", 50, 169, 15, searchText[0] ? C_TEXT : C_MUTED);
    if (pointIn(sr) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) searchEdit = true;

    if (searchEdit) {
        int ch = GetCharPressed();
        while (ch > 0) {
            int n = strlen(searchText);
            if (ch >= 32 && ch <= 125 && n < 98) {
                searchText[n] = (char)ch;
                searchText[n+1] = 0;
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int n = strlen(searchText);
            if (n) searchText[n-1] = 0;
        }
        if (IsKeyPressed(KEY_ENTER)) searchEdit = false;
    }
    if (button("CLEAR", (Rectangle){440,157,90,42}, C_PANEL, C_TEXT)) searchText[0] = 0;

    /* list of active found items that match the search */
    int y = 220, shown = 0;
    for (int i = 0; i < foundCount; i++) {
        FoundItem *f = &foundItems[i];
        if (!statusIsActive(f->status)) continue;
        if (!containsIgnoreCase(f->name, searchText) && !containsIgnoreCase(f->location, searchText)) continue;
        if (y > 650) break;

        panel((Rectangle){38,(float)y,1104,76});
        DrawText(TextFormat("%s  |  %s", categories[f->category], f->name), 58, y + 12, 18, C_NAVY);
        DrawText(TextFormat("Found at: %s   •   %s", f->location, f->date), 58, y + 39, 14, C_MUTED);
        DrawText(statusName(f->status), 760, y + 18, 14, f->status == ST_AVAILABLE ? C_GREEN : C_AMBER);
        DrawText(TextFormat("ID: %d", f->id), 930, y + 18, 13, C_MUTED);
        y += 88;
        shown++;
    }
    if (!shown) DrawText("No active found items match your search.", 58, 240, 17, C_MUTED);

    if (button("BACK", (Rectangle){38,672,150,42}, C_PANEL, C_TEXT)) page = PAGE_HOME;
}

static void drawMatches(void) {
    header("Possible matches for your lost-item report");
    if (selectedLost < 0 || selectedLost >= lostCount) {
        page = PAGE_HOME;
        return;
    }
    LostItem *l = &lostItems[selectedLost];
    DrawText("Possible matches", 38, 112, 27, C_NAVY);
    DrawText(TextFormat("Lost: %s (%s)  |  Location: %s", l->name, categories[l->category], l->location), 38, 151, 15, C_MUTED);

    int y = 198, shown = 0;
    int scores[MAX_ITEMS], indices[MAX_ITEMS], n = 0;

    /* collect every found item that scores 35 or more */
    for (int i = 0; i < foundCount; i++) {
        int s = matchScore(l, &foundItems[i]);
        if (s >= 35) {
            scores[n] = s;
            indices[n] = i;
            n++;
        }
    }

    /* simple sort, best match first */
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[i]) {
                int t = scores[i]; scores[i] = scores[j]; scores[j] = t;
                t = indices[i]; indices[i] = indices[j]; indices[j] = t;
            }
        }
    }

    for (int k = 0; k < n && y < 630; k++) {
        FoundItem *f = &foundItems[indices[k]];
        panel((Rectangle){38,(float)y,1104,100});
        DrawText(TextFormat("%s — %s", categories[f->category], f->name), 58, y + 12, 19, C_NAVY);
        DrawText(TextFormat("Location: %s | Date: %s", f->location, f->date), 58, y + 43, 14, C_MUTED);
        DrawText(TextFormat("Possible match: %d%%", scores[k]), 780, y + 15, 15, scores[k] >= 70 ? C_GREEN : C_AMBER);
        DrawText("Contact details hidden until verification.", 780, y + 42, 12, C_MUTED);
        if (button("REQUEST VERIFY", (Rectangle){940,(float)y+57,180,32}, C_BLUE, RAYWHITE)) {
            sendVerificationRequest(indices[k], selectedLost);
        }
        y += 112;
        shown++;
    }
    if (!shown) DrawText("No possible matches yet. Your report is saved; check again later.", 58, 220, 17, C_MUTED);

    if (button("BACK TO HOME", (Rectangle){38,672,180,42}, C_PANEL, C_TEXT)) page = PAGE_HOME;
}

static void drawReports(void) {
    header("Your reports and conversations (demo profile)");
    DrawText("My reports", 38, 112, 27, C_NAVY);
    DrawText("Records are filtered by the selected demo student profile.", 38, 148, 14, C_MUTED);

    int y = 190, shown = 0;
    for (int i = 0; i < lostCount && y < 630; i++) {
        if (isOwnedByCurrent(lostItems[i].studentId)) {
            panel((Rectangle){38,(float)y,1104,65});
            DrawText(TextFormat("LOST #%d  %s", lostItems[i].id, lostItems[i].name), 55, y + 10, 17, C_NAVY);
            DrawText(statusName(lostItems[i].status), 820, y + 17, 14, C_AMBER);
            y += 74;
            shown++;
        }
    }
    for (int i = 0; i < foundCount && y < 630; i++) {
        if (isOwnedByCurrent(foundItems[i].studentId)) {
            panel((Rectangle){38,(float)y,1104,65});
            DrawText(TextFormat("FOUND #%d  %s", foundItems[i].id, foundItems[i].name), 55, y + 10, 17, C_NAVY);
            DrawText(statusName(foundItems[i].status), 820, y + 17, 14, C_GREEN);
            y += 74;
            shown++;
        }
    }
    if (!shown) DrawText("No reports for this profile yet.", 58, 205, 17, C_MUTED);

    if (button("BACK", (Rectangle){38,672,150,42}, C_PANEL, C_TEXT)) page = PAGE_HOME;
}

static void drawChat(void) {
    header("Private verification conversation — local demo");
    if (selectedChatFound < 0 || selectedChatLost < 0) {
        page = PAGE_HOME;
        return;
    }
    FoundItem *f = &foundItems[selectedChatFound];
    LostItem *l = &lostItems[selectedChatLost];
    DrawText(TextFormat("Verification: %s", f->name), 38, 112, 25, C_NAVY);
    DrawText(TextFormat("Found report #%d  |  Lost report #%d", f->id, l->id), 38, 148, 14, C_MUTED);

    /* message area */
    panel((Rectangle){38,184,1104,365});
    int y = 204;
    for (int i = 0; i < messageCount; i++) {
        if (messages[i].foundId == f->id && messages[i].lostId == l->id && y < 515) {
            Message *m = &messages[i];
            Color cc = !strcmp(m->kind, "SYSTEM") ? C_AMBER : C_BLUE;
            DrawText(TextFormat("%s [%s]:", m->sender, m->kind), 58, y, 13, cc);
            DrawText(m->text, 58, y + 19, 15, C_TEXT);
            y += 53;
        }
    }

    /* input box (formFocus 20 is used to mark the chat box as active) */
    static char chatInput[MAX_DESC] = "";
    Rectangle in = {38,570,800,42};
    DrawRectangleRounded(in, 0.12f, 6, C_PANEL);
    DrawRectangleRoundedLines(in, 0.12f, 6, C_BORDER);
    DrawText(chatInput[0] ? chatInput : "Type a question or answer...", 50, 582, 15, chatInput[0] ? C_TEXT : C_MUTED);
    if (pointIn(in) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) formFocus = 20;

    if (formFocus == 20) {
        int ch = GetCharPressed();
        while (ch > 0) {
            int n = strlen(chatInput);
            if (ch >= 32 && ch <= 125 && n < MAX_DESC - 2) {
                chatInput[n] = (char)ch;
                chatInput[n+1] = 0;
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int n = strlen(chatInput);
            if (n) chatInput[n-1] = 0;
        }
    }

    if (button("SEND", (Rectangle){855,570,120,42}, C_BLUE, RAYWHITE)) {
        if (chatInput[0]) {
            const char *sender = isOwnedByCurrent(f->studentId) ? f->owner : currentUser.name;
            addMessage(f->id, l->id, sender, "MESSAGE", chatInput);
            chatInput[0] = 0;
        }
    }
    if (button("ASK QUESTION", (Rectangle){38,630,175,38}, C_PANEL, C_TEXT)) {
        if (chatInput[0]) {
            addMessage(f->id, l->id, f->owner, "QUESTION", chatInput);
            chatInput[0] = 0;
        } else {
            setNotice("Type your verification question in the message field first.");
        }
    }
    if (button("CONFIRM ANSWERS", (Rectangle){228,630,190,38}, C_GREEN, RAYWHITE)) approveVerification(1);
    if (button("INCORRECT", (Rectangle){433,630,140,38}, C_REDBG, C_RED)) approveVerification(0);
    if (button("HANDOVER PENDING", (Rectangle){588,630,190,38}, C_AMBERBG, C_AMBER)) startReturn();
    if (button("MARK RETURNED", (Rectangle){793,630,170,38}, C_GREEN, RAYWHITE)) confirmReturn = true;
    if (button("BACK", (Rectangle){978,630,164,38}, C_PANEL, C_TEXT)) {
        page = PAGE_HOME;
        formFocus = -1;
    }

    /* confirmation popup for marking an item as returned */
    if (confirmReturn) {
        DrawRectangle(0, 0, W, H, (Color){0,0,0,100});
        panel((Rectangle){350,285,480,180});
        DrawText("Confirm item return?", 390, 315, 23, C_NAVY);
        DrawText("This removes the item from active suggestions.", 390, 353, 14, C_MUTED);
        if (button("YES, RETURNED", (Rectangle){390,395,190,42}, C_GREEN, RAYWHITE)) markReturned();
        if (button("CANCEL", (Rectangle){600,395,150,42}, C_PANEL, C_TEXT)) confirmReturn = false;
    }
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(W, H, "FAST Lost & Found");
    SetTargetFPS(60);

    ensureDataFiles();
    loadData();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (noticeTimer > 0) noticeTimer -= dt;

        BeginDrawing();
        ClearBackground(C_BG);

        switch (page) {
            case PAGE_HOME:      drawHome();    break;
            case PAGE_LOST_FORM:
            case PAGE_FOUND_FORM:drawForm();    break;
            case PAGE_BROWSE:    drawBrowse();  break;
            case PAGE_REPORTS:   drawReports(); break;
            case PAGE_MATCHES:   drawMatches(); break;
            case PAGE_CHAT:      drawChat();    break;
        }

        drawNotice();
        EndDrawing();
    }

    saveFound();
    saveLost();
    saveMessages();
    CloseWindow();
    return 0;
}
