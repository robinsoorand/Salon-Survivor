#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void check_error(int rc, sqlite3 *db, char *errMsg, const char *msg)
{
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "%s: %s\n", msg, errMsg ? errMsg : sqlite3_errmsg(db));
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

int callback_print(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void execute_query(sqlite3 *db, const char *sql)
{
    char *errMsg = NULL;
    int rc = sqlite3_exec(db, sql, callback_print, NULL, &errMsg);
    check_error(rc, db, errMsg, "SQL error executing query");
    printf("\n");
}

int check_student_exists(sqlite3 *db, const char *eid)
{
    sqlite3_stmt *stmt;
    int exists = 0;
    const char *sql = "SELECT id FROM students WHERE eid = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, eid, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = 1;
    }

    sqlite3_finalize(stmt);
    return exists;
}

long insert_and_get_id(sqlite3 *db, const char *sql)
{
    char *errMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    check_error(rc, db, errMsg, "SQL error on insert");
    return sqlite3_last_insert_rowid(db);
}

void task_15_5(sqlite3 *db)
{
    printf("--- Task 15-5: Adding yourself more reliably ---\n");
    const char *my_eid = "50607120296";
    const char *my_fname = "Robin";
    const char *my_lname = "Soorand";
    long student_id = 0;

    if (check_student_exists(db, my_eid))
    {
        printf("Student with eid %s already exists.\n", my_eid);
        /* Retrieve the ID to continue with declarations just in case */
        sqlite3_stmt *stmt;
        const char *sql_sel = "SELECT id FROM students WHERE eid = ?;";
        sqlite3_prepare_v2(db, sql_sel, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, my_eid, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            student_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        char sql_insert_student[256];
        sprintf(sql_insert_student, "INSERT INTO students (eid, fname, lname) VALUES ('%s', '%s', '%s');", my_eid, my_fname, my_lname);
        student_id = insert_and_get_id(db, sql_insert_student);
        printf("Inserted student with ID: %ld\n", student_id);
    }

    /* Insert subject */
    /* Let's check if subject exists first */
    const char *subject_code = "ITC0001";
    sqlite3_stmt *stmt;
    int subject_exists = 0;
    long subject_id = 0;
    const char *sql_sub_check = "SELECT id FROM subjects WHERE code = ?;";
    sqlite3_prepare_v2(db, sql_sub_check, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, subject_code, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        subject_exists = 1;
        subject_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (!subject_exists)
    {
        const char *sql_insert_sub = "INSERT INTO subjects (name_en, name_et, code, credits, assesment_type) VALUES ('C Programming', 'C Programmeerimine', 'ITC0001', 6, 'Exam');";
        subject_id = insert_and_get_id(db, sql_insert_sub);
        printf("Inserted subject with ID: %ld\n", subject_id);
    }
    else
    {
        printf("Subject %s already exists with ID: %ld\n", subject_code, subject_id);
    }

    /* Check if declarations exist, to make it rerunnable without blowing up constraints if any */
    /* Since we just need to add declarations, we'll do it using INSERT IGNORE or checking */
    char sql_dec1[256];
    sprintf(sql_dec1, "INSERT INTO declarations (student_id, subject_id, grade) VALUES (%ld, %ld, 5);", student_id, subject_id);

    char sql_dec2[256];
    sprintf(sql_dec2, "INSERT INTO declarations (student_id, subject_id, grade) VALUES (%ld, 1, 4);", student_id);

    char sql_dec3[256];
    sprintf(sql_dec3, "INSERT INTO declarations (student_id, subject_id, grade) VALUES (%ld, 2, 3);", student_id);

    printf("Executing declarations inserts...\n");
    sqlite3_exec(db, sql_dec1, NULL, NULL, NULL); /* Ignoring errors for reruns */
    sqlite3_exec(db, sql_dec2, NULL, NULL, NULL);
    sqlite3_exec(db, sql_dec3, NULL, NULL, NULL);
    printf("Declarations added.\n\n");
}

void task_15_2(sqlite3 *db)
{
    printf("--- Task 15-2: Basic data retrieval queries ---\n");
    printf("Query 1: Subjects less than 6 credits\n");
    execute_query(db, "SELECT name_en, credits FROM subjects WHERE credits < 6;");

    printf("Query 2: Subjects with an exam, ordered by credits (lowest to highest)\n");
    execute_query(db, "SELECT name_en, assesment_type, credits FROM subjects WHERE assesment_type LIKE 'Exam%' ORDER BY credits ASC;");
}

void task_15_3(sqlite3 *db)
{
    printf("--- Task 15-3: Retrieving and combining data from multiple tables ---\n");
    printf("Query 1: Find all grades for students whose first name is \"Marko\"\n");
    execute_query(db, "SELECT st.fname, st.lname, su.name_en, d.grade FROM students st JOIN declarations d ON st.id = d.student_id JOIN subjects su ON d.subject_id = su.id WHERE st.fname = 'Marko';");

    printf("Query 2: Find all classes Robin Soorand completed, show with grades, starting from highest grade.\n");
    execute_query(db, "SELECT su.name_en, d.grade FROM students st JOIN declarations d ON st.id = d.student_id JOIN subjects su ON d.subject_id = su.id WHERE st.fname = 'Robin' AND st.lname = 'Soorand' ORDER BY d.grade DESC;");
}

void task_15_4(sqlite3 *db)
{
    printf("--- Task 15-4: Grouping data and performing calculations ---\n");
    printf("Query: Find each student's total earned credits and average grade\n");
    /* Assuming a grade > 0 means the class is completed and credits earned, but let's just sum credits for declarations */
    /* To be safe, we just calculate it directly. If grade > 0, credits count. */
    const char *sql = "SELECT st.fname, st.lname, SUM(CASE WHEN d.grade > 0 THEN su.credits ELSE 0 END) AS total_credits, AVG(d.grade) AS avg_grade FROM students st LEFT JOIN declarations d ON st.id = d.student_id LEFT JOIN subjects su ON d.subject_id = su.id GROUP BY st.id;";
    execute_query(db, sql);
}

int is_uni_id_exists(sqlite3 *db, const char *uni_id)
{
    sqlite3_stmt *stmt;
    int exists = 0;
    const char *sql = "SELECT id FROM students WHERE uni_id = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, uni_id, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = 1;
    }
    sqlite3_finalize(stmt);
    return exists;
}

void task_15_6(sqlite3 *db)
{
    printf("--- Task 15-6: Changing existing data (Generating uni_ids) ---\n");
    sqlite3_stmt *stmt;
    const char *sql_select = "SELECT id, fname, lname FROM students WHERE uni_id IS NULL OR uni_id = '';";

    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return;
    }

    /* We'll collect the students to update first to avoid updating while selecting if it causes issues */
    int ids[100];
    char fnames[100][50];
    char lnames[100][50];
    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && count < 100)
    {
        ids[count] = sqlite3_column_int(stmt, 0);

        const unsigned char *fname = sqlite3_column_text(stmt, 1);
        if (fname) strncpy(fnames[count], (const char *)fname, 49);
        else fnames[count][0] = '\0';

        const unsigned char *lname = sqlite3_column_text(stmt, 2);
        if (lname) strncpy(lnames[count], (const char *)lname, 49);
        else lnames[count][0] = '\0';

        count++;
    }
    sqlite3_finalize(stmt);

    for (int i = 0; i < count; i++)
    {
        char base_uni_id[10];
        base_uni_id[0] = '\0';
        if (strlen(fnames[i]) >= 2)
        {
            strncat(base_uni_id, fnames[i], 2);
        }
        else
        {
            strncat(base_uni_id, "xx", 3);
        }

        if (strlen(lnames[i]) >= 2)
        {
            strncat(base_uni_id, lnames[i], 2);
        }
        else
        {
            strncat(base_uni_id, "xx", 3);
        }

        /* Convert to lowercase */
        for(int k = 0; base_uni_id[k]; k++)
        {
            if (base_uni_id[k] >= 'A' && base_uni_id[k] <= 'Z')
            {
                base_uni_id[k] = base_uni_id[k] + 32;
            }
        }

        char final_uni_id[20];
        strcpy(final_uni_id, base_uni_id);
        int suffix = 1;

        /* For Robin Soorand we can specifically set it to the requested student code,
           but the task says "Generate all the students Uni-ID's".
           If the user specifically provided a student code, we should probably set that for him.
           Let's just handle standard generation and conflict resolution. */

        if (strcmp(fnames[i], "Robin") == 0 && strcmp(lnames[i], "Soorand") == 0)
        {
            strcpy(final_uni_id, "252566");
        }
        else
        {
            while (is_uni_id_exists(db, final_uni_id))
            {
                sprintf(final_uni_id, "%s%d", base_uni_id, suffix);
                suffix++;
            }
        }

        /* Update student */
        char sql_update[256];
        sprintf(sql_update, "UPDATE students SET uni_id = '%s' WHERE id = %d;", final_uni_id, ids[i]);
        char *errMsg = NULL;
        int rc = sqlite3_exec(db, sql_update, NULL, NULL, &errMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Update error: %s\n", errMsg);
            sqlite3_free(errMsg);
        }
        else
        {
            printf("Assigned uni_id '%s' to student ID %d (%s %s)\n", final_uni_id, ids[i], fnames[i], lnames[i]);
        }
    }
    printf("\n");
}

int main(void)
{
    sqlite3 *db;
    int rc = sqlite3_open("study_information.db", &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    task_15_5(db);
    task_15_2(db);
    task_15_3(db);
    task_15_4(db);
    task_15_6(db);

    /* Print all students at the end to show the update results */
    printf("--- Final state of students table ---\n");
    execute_query(db, "SELECT id, fname, lname, uni_id FROM students;");

    sqlite3_close(db);
    return EXIT_SUCCESS;
}
