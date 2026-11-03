# Testing and Maintenance

---

## 1. About Software Testing

- Software Testing is a method to check whether the actual software product matches expected requirements and to ensure that the software product is defect free.
- The purpose of software testing is to identify errors, gaps or missing requirements in contrast to actual requirements.
- It is a process of identifying the correctness of software by considering all its attributes: Reliability, Scalability, Portability, Re-usability & Usability of the product.
- Testing is mandatory because it would be a dangerous situation if the software fails at any time due to lack of testing. Without testing, software cannot be deployed to the end user.
- ✔ Software Testing is done at every phase level in the software development life cycle (SDLC).
- ✔ Performed by: Software Tester, Software Developer, Project Manager, End-User.

---

## 2. Types of Software Testing

```
Types of Software Testing
├── Manual
│   ├── White Box Testing
│   ├── Black Box Testing
│   │   ├── Functional
│   │   │   ├── Unit Testing
│   │   │   ├── Integration
│   │   │   ├── System Testing
│   │   │   └── User Acceptance Testing
│   │   └── Non-Functional
│   │       ├── Compatibility Testing
│   │       ├── Performance Testing
│   │       └── Usability Testing
│   └── Gray Box Testing
└── Automation
```

---

## 3. What is a Test Case?

- A test case includes a set of actions or conditions, using which a testing engineer can compare expected result and actual results.
- They determine whether a software product is functioning as per the requirements of the customer.

**Example test case (Login):**

| Field | Value |
| :--- | :--- |
| Test Scenario ID | Login-1 |
| Test Case ID | Login-1A |
| Test Case Description | Login – Positive test case |
| Test Priority | High |
| Pre-Requisite | A valid user account |
| Post-Requisite | NA |

**Test Execution Steps:**

| S.No | Action | Inputs | Expected Output | Actual Output | Test Browser | Test Result | Test Comments |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | Launch application | https://www.facebook.com/ | Facebook home | Facebook home | IE-11 | Pass | [Priya 10/17/2017 11:44 AM]: Launch successful |
| 2 | Enter correct Email & Password and hit login button | Email id: test@xyz.com, Password: ****** | Login success | Login success | IE-11 | Pass | [Priya 10/17/2017 11:45 AM]: Login successful |

---

## 4. Principles of Software Testing

The 7 Testing Principles:

1. Testing shows presence of defects
2. Exhaustive testing is impossible
3. Early testing
4. Defect clustering
5. Pesticide paradox
6. Testing is context dependent
7. Absence of errors fallacy

### 1. Testing Shows the Presence of Defects
- The primary purpose is to find any defects that might cause the product failure.
- The testing team cannot say that the product is 100% error-free even though testing is done.
- It reduces the number of undiscovered defects in the application.
- Therefore, design the needed test cases to find as many defects as possible.

### 2. Exhaustive Testing is Impossible
- Sometimes it seems very hard to test all the modules and their features.
- So, instead of performing exhaustive testing, they use important testing criteria such as risk analysis and priority of modules.
- Then the product can be delivered on schedule.

### 3. Early Testing
- The testing team will have a deep understanding of the product as they were involved from the beginning of the requirement gathering and analysis phase.
- If errors are found in an initial stage of the development life cycle, they are easier to fix, and time and cost are saved.
- Otherwise, if found late, the whole system process may need to change.

### 4. Defect Clustering
- A small number of modules contain most of the defects detected.
- As per Pareto's Principle or the 80-20 Rule, 80% of defects are caused by 20% of code.
- If testers focus on a small area, they may fail to find bugs in other areas.

### 5. Pesticide Paradox
- If the same set of test cases is used again and again over a period of time, these tests will not be able to find new bugs in the software.
- So new and different tests need to be written for multiple parts of the software, which helps find more bugs.

### 6. Testing is Context-Dependent
- There are multiple fields such as e-commerce, commercial websites, banking applications etc. These projects and products include different elements, features & requirements.
- So, different types of products can be tested differently based on their nature.
- The same test cases cannot be applied to different projects.

### 7. Absence of Errors Fallacy
- Once the application is completely tested and no bugs are identified before the release, we can say the application is 99 percent bug-free.
- Finding and fixing defects doesn't help if the system is unusable or does not meet users' needs and expectations.
- Absence of errors fallacy means identifying and fixing bugs would not help if the application is impractical and not able to accomplish the client's requirements and needs.

---

## 5. White Box Testing

- White box testing techniques analyze the internal design, code structure & working of the complete software product.
- It is also called Clear box testing, Open box testing, Transparent box testing, Code-based testing and Glass box testing.
- The tester has knowledge about the internal structure or the programming of the software.
- It is mostly done by software developers.
- Performed with Unit Testing & Integration Testing.
- **White Box Testing Tools:** EclEmma, NUnit, PyUnit, HTMLUnit, CppUnit

*(Diagram: Input → [White Box showing internal flow] → Output)*

### What do you verify in White Box Testing?
- ✔ Internal security of code
- ✔ Poorly structured paths in the coding processes
- ✔ The flow of specific inputs through the code
- ✔ Loops, decision conditions
- ✔ Each statement of your program
- ✔ Functions & modules on an individual basis
- ✔ Expected output

### White Box Testing Techniques

**1. Path Coverage / Testing**
- Testing is dependent on the program's control or flow structure.
- All possible paths are defined with entry to exit points in the system.

*(Diagram: Main() → Function A, Function B; Function A → Function C, Function D; Function D → Function E, Function F; Function F → Function H, Function I; Function B → Function G)*

**2. Loop Testing**
- There are loops such as While, For and Do-while loops etc.
- It checks all Simple, Nested & Concatenated loops.
- It checks whether the ending condition is working correctly and whether the size of the conditions is enough.

```c
while (condition 1) {
    statement(s);
}
while (condition 2) {
    statement(s);
}
```

**3. Branch Coverage / Condition Testing**
- The logical conditions for every value are checked, whether true or false.
- This means that both the if and else conditions are verified.

**4. Statement Coverage**
- Statement coverage is a testing technique in which the tester traverses all statements in the code.
- It ensures that each statement of the code is executed at least once.
- Hence each line of the code is verified.

```
1  Prints (int a, int b) {
2      int result = a + b;
3      If (result > 0)
4          Print ("Positive", result)
5      Else
6          Print ("Negative", result)
7  }
```

### Advantages & Disadvantages of White Box Testing

**Advantages**
1. White box testing optimizes code so hidden errors can be identified.
2. Test cases of white box testing can be easily automated.
3. Early detection of errors that improve code quality.
4. It covers most paths & code of the software.

**Disadvantages**
1. White box testing can be quite complex, expensive & time-consuming.
2. Requires professional resources with detailed understanding of programming.
3. Missing functionalities cannot be detected.
4. Redesign of code needs test cases to be written again.

---

## 6. Black Box Testing

- In Black Box Testing, the functionalities of software applications are tested without knowledge of the internal code structure, implementation details and internal paths.
- The tester gives input values to examine functionality & checks whether the function is giving the expected output or not.
- If the function produces correct output, it passes the test, otherwise it fails.
- It is also known as Behavioral Testing, Functional & Closed box Testing.
- Performed by Software Testers.
- **Black Box Testing Tools:** QTP, Selenium, LoadRunner & JMeter

*(Diagram: Input → [Black Box] → Output)*

### Types of Black Box Testing
*(Same hierarchy as the "Types of Software Testing" tree in section 2: Functional — Unit, Integration, System, User Acceptance; Non-Functional — Compatibility, Performance, Usability.)*

### Black Box Testing Techniques

**1. Equivalence Partitioning**
- Input values provided to the system are divided into different classes or groups based on similarity in outcome.
- Instead of using each & every input value, use any one value from the group to test the outcome.

Example: AGE field accepts value 18 to 56.

| Invalid | Valid | Invalid |
| :---: | :---: | :---: |
| <= 17 | 18–56 | >= 57 |

**2. Boundary Value Analysis**
- Boundary values are those that contain the upper and lower limit of a variable.
- It tests, while entering boundary values, whether the software is producing correct output or not.

Example: AGE field accepts value 18 to 56.

| Invalid (min − 1) | Valid (min, min+1, max−1, max) | Invalid (max + 1) |
| :---: | :---: | :---: |
| 17 | 18, 19, 55, 56 | 57 |

**3. Decision Table Testing**
- Various input combinations & their respective system behavior are captured in tabular form.
- Checks the logical relationship between two or more inputs. Ex. Gmail Account.

| | Case 1 | Case 2 | Case 3 | Case 4 |
| :--- | :---: | :---: | :---: | :---: |
| Email (condition 1) | T | T | F | F |
| Password (condition 2) | T | F | T | F |
| Expected Result (Action) | Account Page | Incorrect password | Incorrect email | Incorrect email |

**4. Error Guessing**
- Based on the experience of the tester, where the tester uses experience to guess the problematic areas of the software.
- **Examples:** Divide by zero, handling null values in text fields, accepting the Submit button without any value, file upload without attachment, file upload with less than or more than the limit size.

**5. State Transition Testing**
- Used to capture behavior of the software application when different input values are given to the same function.
- Applies to those types of applications that provide a specific number of attempts to access the application.

*(Diagram – ATM PIN example: start → card inserted → wait for PIN → enter PIN → 1st try → (PIN OK → access to account | PIN not OK → 2nd try) → (PIN OK → access | PIN not OK → 3rd try) → (PIN OK → access | PIN not OK → eat card))*

**6. All Pairs Testing Technique**
- Used to test all the possible discrete combinations of values.
- This combinational method is used for testing applications that use checkbox input, radio button input, list box, text box etc.

### Advantages & Disadvantages of Black Box Testing

**Advantages**
1. The tester does not need to have more functional knowledge or programming skills.
2. It is efficient for implementing the tests in a larger system.
3. Tests are executed from the user's or client's point of view.
4. It is used in finding ambiguity and contradictions in the functional specifications.

**Disadvantages**
1. Without clear programming knowledge, test cases are difficult to implement.
2. It does not reveal errors in the control structure.
3. Working with a large sample space of inputs can be exhaustive and consumes a lot of time.

---

## 7. Black Box vs White Box Testing

| No. | Black Box Testing | White Box Testing |
| :---: | :--- | :--- |
| 1 | The internal structure & working of the software is not known to the tester. Focus on input & output of product. | The internal structure & working of the software is known to the tester. |
| 2 | Also known as Functional Testing, Data-driven testing, Closed-box testing & Behavioral Testing. | Also known as Structural testing, Glass box testing, Code-based testing, and Transparent testing. |
| 3 | Less programming knowledge is required. | Requirement of complete programming knowledge. |
| 4 | Done at higher levels of testing that are system and acceptance testing. | Done at lower levels of testing that are unit testing and integration testing. |
| 5 | Mainly performed by Software Testers. | Mainly performed by Developers. |
| 6 | Less time-consuming. | More time-consuming. |
| 7 | The base of this testing is external expectations. | The base of this testing is coding, which is responsible for internal working. |
| 8 | Its main objective is to fulfill customer need. | Its main objective is to check the code quality. |
| 9 | Defects are identified once the code is ready. | Defects are identified at an early stage. |
| 10 | Types: Functional & Non-Functional Testing and Regression testing. | Types: Path testing, Loop testing and Condition testing. |
| 11 | Can be done by trial and error ways and methods. | Data domains along with inner or internal boundaries can be better tested. |
| 12 | Example: Search something on Google by using keywords. | Example: By input to check and verify loops. |

---

## 8. Unit Testing

- Unit testing is the first level of testing, done before the other remaining levels of testing.
- In Unit Testing, individual units or components of software are tested.
- A unit may be an individual function, method, procedure, module or object in a program.
- Every module of the project is checked one by one, which is also called component testing. It uses white box testing techniques.
- It is done during the coding phase of SDLC by Software Developers.
- **Unit Testing Tools:** JUnit, NUnit, PHPUnit, EMMA & JMockit

*(Diagram: Modules A, B, C tested individually in Unit Testing, then combined under Integration Testing. Level stack, bottom to top: Unit Testing → Integration Testing → System Testing → Acceptance Testing.)*

### Why Unit Testing?
1. To understand correctness of code & enable changes to be made quickly.
2. To test every function and procedure in the project.
3. Helps to fix bugs early in the development cycle and save costs.
4. It helps in documentation.
5. It helps with code re-use. Migrate both your code & tests to your new project.
6. Makes faster & efficient software.

**Unit Testing in the OO Context:** In a program, there are different packages & classes which contain a number of different operations like functions, methods, subclasses, derived classes, private, public & protected attributes etc.

### Example of Unit Testing

A Server has modules M1 (Amount Transfer), M2 (Loan), M3 (Sale). The *Amount Transfer* module has components: FAN field, TAN field, Amount field, Transfer button, Cancel button.

**For From Account Number (FAN) & To Account Number (TAN):**

| Values | Description |
| :--- | :--- |
| 1234 | Account Number Accepted |
| 4300 | Error: Account Number Invalid |
| Blank | Error: Enter Account Number |
| Alphanumeric | Error: Enter digits only |
| Block Acc. No | Error Message |

**For Transfer Button:** Enter valid FAN value → Enter valid TAN value → Enter correct amount → Click on Transfer button.

**For Cancel Button:** Enter value of FAN, TAN & Amount → Click on Cancel button: all data should be cleared.

### Advantages of Unit Testing
1. Due to the modular nature of unit testing, the tester tests parts of the project without waiting for others to be completed.
2. The development team focuses on the functionality of the unit and how the functionality should work.
3. It allows developers to detect and fix issues early in the development process, before they become larger and more difficult to fix.
4. It ensures that each unit of code meets the requirements, improving the overall quality of the software.
5. Unit testing enables developers to work faster and more efficiently.

### Disadvantages of Unit Testing
1. Unit testing requires more time and effort to create and maintain the test cases, especially for complex systems.
2. Unit testing may not be sufficient for testing interactions between units, as it only focuses on individual units.
3. Unit testing requires ongoing maintenance and updates, as the code and test cases must be kept up-to-date with changes to the software.

---

## 9. Integration Testing

- Integration testing is the second level of the software testing process; it comes after unit testing.
- All modules may be coded by different programmers or developers.
- Integrating various components or modules of an application & testing their behavior as a single unit or group is called Integration Testing.
- Also known as Thread testing or String testing.
- The goal of Integration Testing is to check correctness & communication among all modules.
- **Integration Testing Tools:** Selenium, PyTest, JUnit, Jasmine, Steam, Mockito etc.

### Why Integration Testing?
1. Each module is designed by a different software developer whose programming logic may differ from developers of other modules, so integration testing becomes essential to determine the working of software modules.
2. To check the interaction of software modules with the database, whether it is correct or not.
3. Customer requirements can change at the time of module development. These new requirements may not be tested at unit testing, hence integration testing becomes mandatory.
4. Incompatibility between modules of software could create errors.
5. To test hardware's compatibility with software.

### Example of Integration Testing

**After User 1 logs in to Gmail account**
- User 1: Click on Compose mail, fill details & send mail to User 2.
- If mail is not sent, save in 'Draft', otherwise save in 'Sent Items'. Log out of the account.

**After User 2 logs in to Gmail account**
- User 2: Move to the 'Inbox' and verify that the mail has arrived. Open mail & check. If needed, reply to it. Log out of the account.

### Types of Integration Testing

```
Integration testing
├── Incremental Integration Testing
│   ├── Top-Down Integration Testing
│   └── Bottom-Up Integration Testing
└── Non-Incremental Integration Testing
```

**1. Incremental Integration Testing**
- Modules are added in ascending order one by one or according to need.
- The selected modules must be logically related.
- They test data flow between modules & correctness of each function.
- The process continues until the successful testing of all the modules.
- It has two types: Top-Down & Bottom-Up.
- **Example:** Flipkart application, flow of testing: Flipkart → Login → Home → Search → Add cart → Payment → Logout

**1.1 Top-Down Integration Testing**
- It starts by testing the top-most modules & moves down to the lowest set of modules one by one & checks the control flow of the system.
- There is a possibility that lower-level modules might not have been developed while top modules are tested.
- It uses **Stubs** — dummy programs which act as substitutes for the missing modules in the testing.
- Critical modules are tested on priority & major design flaws are detected as early as possible.

*(Diagram: M1 → M2, M3, M4; M3 → M5; M4 → M6, M7. Integration order: M1 → M1+M2+M3+M4 → +M5 → +M6, M7.)*

**1.2 Bottom-Up Integration Testing**
- It starts with testing the lowest units of the application and moves up one by one.
- Testing takes place from the bottom of the control flow upwards.
- Again, it's possible that higher-level modules might not have been developed by the time lower modules are tested.
- It uses **Drivers** — dummy programs which act as substitutes for the missing modules in the testing.
- Several disjoint subsystems can be tested simultaneously & test results are easy to observe.

**2. Non-Incremental Integration Testing / Big Bang Testing**
- Once all the modules are developed and tested individually, they are integrated once and tested together at once. No parent & child concept.
- The only advantage of this type of testing is that it is very suitable for smaller systems.
- If an error is found, it is very difficult to resolve because it may belong to any of the modules being integrated.
- The goal is to verify the overall functionality of the system and to identify any integration problems that arise when all components are combined.

---

## 10. System Testing

- System Testing validates the complete and fully integrated software product.
- The purpose of a system test is to evaluate the end-to-end system specifications.
- It's a type of black box testing performed by the Quality Assurance Team in the testing phase.
- It focuses on the Functionality, Accuracy, Quality, Expected Output & Overall behavior of an application rather than the inner working of the system.
- **System Testing Tools:** Selenium, LoadRunner, JMeter, Microsoft Test Manager, SoapUI
- The choice of tool depends on various factors like technology used, size of the project, budget & testing requirements etc.

### Importance of System Testing
1. **Improved Product Quality:** It tests that the product can successfully work across different platforms and environments.
2. **Error Reduction:** It verifies a system's code & functionality as per requirements, so errors that weren't detected during integration and unit testing can be exposed during system testing.
3. **Cost Savings:** Timely & continuous system testing reduces unexpected costs & project delays.
4. **Security:** It ensures that the tested system doesn't contain potential vulnerabilities or bugs that can put end users' system data at risk.
5. **Customer Satisfaction:** It builds customer confidence & improves overall user experience.
6. **Software Performance:** It tracks complete system performance. It also helps to understand changes in a system's performance & behavior, such as memory consumption, CPU utilization etc., and informs developers to take proactive action.

### Types of System Testing
1. Performance Testing
2. Load Testing
3. Usability Testing
4. Regression Testing
5. Migration Testing
6. Functional Testing
7. Recovery Testing
8. Stress Testing
9. Software & Hardware Testing

**1. Performance Testing:** Measures speed, load time, stability, reliability & response times of the system under various conditions.

**2. Load Testing:** Determines how a system or software performs under a real-life extreme load, such as throughput, number of users etc.

**3. Usability Testing:** Evaluates whether the system is easy to use and functional for the end user, such as user error rates, task success rates, time a user takes to complete a task & user satisfaction etc.

**4. Regression Testing**
- Ensures that any changes done during the development process have not introduced new defects or bugs.
- Also ensures that old defects or bugs will not reappear on the addition of new software over time.

**5. Migration Testing:** Ensures that if the system needs to be moved to a new infrastructure, it can be migrated without any issue.

**6. Functional Testing**
- The tester finds out any missing function in the system & makes a list of it.
- It can be added during functional testing and should improve the quality of the system.

**7. Recovery Testing:** Ensures that the system is capable of recovering from certain system errors, crashes and failures.

**8. Stress Testing:** Ensures the robustness of the system under various loads & capacities.

**9. Software & Hardware Testing**
- This testing of the system checks hardware and software compatibility & interaction.
- The hardware configuration must be compatible with the software to run it without any issue.

### System Testing Example

| Test Case Type | Description | Test Step | Expected Result | Status |
| :--- | :--- | :--- | :--- | :---: |
| Functionality | Area should accommodate up to 20 characters | Input up to 20 characters | All 20 characters in the request should be appropriate | Pass or Fail |
| Security | Verify password rules are working | Create a new password in accordance with rules | The user's password will be accepted if it adheres to the rules | Pass or Fail |
| Usability | Ensure all links are working properly | Have users click on various links on the page | Links will take users to another web page according to the on-page URL | Pass or Fail |

---

## 11. Acceptance Testing

- It is the final level of software testing before making the system available for actual use.
- It is a type of Black box testing & is performed by the End user or Client.
- Here, the end-user or customer uses the application to find out whether the software meets the user's expectations and works as expected.
- It is performed in the user/live environment or in real-time scenarios.
- Also called User Acceptance Testing (UAT), Functional Acceptance Testing (FAT), End User Testing or Red Box Testing (RBT).
- **Acceptance Testing Tools:** FitNesse, Watir etc.

*(Diagram: Company develops product → delivers product to Customers → Customers give requirements → Acceptance Testing performed. Level stack: Unit → Integration → System → Acceptance.)*

### Importance of Acceptance Testing
1. Helps identify bugs that may have been left during the development process.
2. Helps to identify that the end product works according to the client's or user's expectations or requirements.
3. Brings confidence and satisfaction to the clients as they are directly involved in the testing process.
4. Helps to deliver the end product without any bugs.
5. To satisfy all the functionalities mentioned in the SRS document.

*(Flow: Development → System Testing → Acceptance Testing → Production)*

### Types of Acceptance Testing

**1. User Acceptance Testing (UAT)**
- Performed from the end-user point of view.
- The client checks whether the software product is working correctly as per the requirements of the user before moving it into the production environment.
- This testing is performed in a different, production-like set-up environment.
- It does not focus on errors & bugs; it mainly focuses on functionality required by the clients.
- This testing is also known as End-User Testing.

**2. Business Acceptance Testing (BAT)**
- It checks whether the software product is able to meet the business requirements & operational need or not, as per the real world.
- It mainly focuses on business risk & financial factors, which are among the challenging things in changing market conditions and the advancement of technologies.

**3. Regulations Acceptance Testing (RAT)**
- It checks whether the software product is developed according to the rules & regulations of the country or region where it is getting released.
- If any product is released in violation of rules and regulations, the software product owner will be considered responsible.

---

## 12. Alpha Testing vs Beta Testing

| No | Alpha Testing | Beta Testing |
| :---: | :--- | :--- |
| 1 | Performed by a team of highly skilled testers who are usually internal employees of the organization. | Performed by clients or end-users at client location or in a real-time environment. |
| 2 | Involves both white box & black-box techniques. | Uses only black-box testing. |
| 3 | Reliability or Security not performed in-depth; focus on bugs, errors etc. | Reliability, Security & Robustness checked during beta testing. |
| 4 | Long execution cycles may be required. | Only a few weeks are required for the execution. |
| 5 | Critical issues can be identified by developers immediately. | Most of the issues or feedback collected from users will be implemented in future versions of the product. |
| 6 | Focus on the product's quality before going to beta testing. | Focus on product's quality & ensures that the product is ready for real-time users. |
| 7 | Performed nearly at the end of the software development. | It is a final test before shipping a product to the customers. |

---

## 13. Verification & Validation

### Example of Verification & Validation — WhatsApp Application

| Verification Process | Validation Process |
| :--- | :--- |
| Means "Are we implementing the software right?" | Means "Have we implemented the right software?" |
| Developer developed and then verifies: ✓ Chatting functionality ✓ Status functionality ✓ Audio call functionality ✓ Video call functionality ✓ Share photos functionality | Tester validates the complete developed product at the same time: ✓ Validate Chatting, Status, Audio Call, Video Call, Share Photos functionality at the same time |

### Verification vs Validation

| No | Verification | Validation |
| :---: | :--- | :--- |
| 1 | Means "Are we implementing the software right?" | Means "Have we implemented the right software?" |
| 2 | Comes before validation | Comes after verification |
| 3 | Known as Static Testing | Known as Dynamic Testing |
| 4 | Executed by Quality Assurance team or Developers | Executed by the Testing team |
| 5 | Includes checking documents, design, code and program | Includes testing and validating the actual product |
| 6 | Checks whether the software conforms to specification | Checks whether the software meets the requirements and expectations of a customer |
| 7 | Does not involve executing the code | Always involves executing the code |
| 8 | Finds bugs early in the development cycle | Can find bugs that the verification process cannot catch |
| 9 | Includes different methods like Inspections, Reviews & Walkthroughs | Includes testing like Functional, System, Integration & User acceptance testing |
| 10 | Developer can verify that the inputs follow the outputs or not | Tester can validate that the user accepts the product or not |

---

## 14. Defect (Bug) and Defect Life Cycle

### What is a Defect or Bug?
- A Defect or Bug is an error in an application that is created during building or designing software, due to which the software starts to show abnormal behavior during its use.
- The variation between the actual results and expected results is known as a Defect.

**Defect/Bug Life Cycle**
- In software testing, it is the specific set of states that a defect or bug goes through in its entire life.
- The purpose of the defect life cycle is to easily coordinate and communicate the current status of a defect and make the defect-fixing process systematic and efficient.

**Performed by:** Developer & Tester
**Tools:** JIRA, Trac, Redmine etc.

### Defect Life Cycle Flow

```
New → Assigned → Open ──(valid bug or not?)──► Duplicate / Rejected / Deferred / Not a bug
                    │
                    ▼
                  Fixed → Retest ──(bug present)──► Reopened ──► Open
                            │
                            ▼
                        Verified → Closed
```

1. **NEW:** Tester identified the defect. Tester sends a proper defect document to the Development team.
2. **ASSIGNED:** Defect is assigned to the Developer team.
3. **OPEN:** Developer team works on the defect to fix the issue. If the developer team feels that the defect is not appropriate, it is transferred to either 'Duplicate', 'Rejected', or 'Deferred' state.
   - **Duplicate:** Defect is repeated or sent twice.
   - **Rejected:** Defect is invalid.
   - **Deferred:** Defect is not of high priority and is expected to get fixed in the next release.
4. **FIXED:** Developer takes necessary actions on coding to fix the defect. The defect will be removed from the application.
5. **RETEST:** Testing team checks whether the bug has been fixed by the developer or not.
6. **REOPENED:** If the defect still exists even after the Developer team has fixed the bug, the lifecycle restarts.
7. **VERIFIED:** The tester re-tests the defect after it got fixed by the Developer team. If the tester does not find any kind of defect, the status assigned is Verified.
8. **CLOSED:** Once the defect has been verified as fixed, the Testing team closes the issue.

---

## 15. Testing vs Debugging

| No | Testing | Debugging |
| :---: | :--- | :--- |
| 1 | Testing is the process to find bugs and errors. | Debugging is the process of correcting the bugs found during testing. |
| 2 | It is done by the Tester. | It is done by either a Programmer or Developer. |
| 3 | Testing can be manual or automated. | Debugging is always manual. |
| 4 | It is based on different testing levels, i.e. unit testing, integration testing, system testing, etc. | Debugging is based on different types of bugs. |
| 5 | Programming knowledge is not required to perform the testing process. | Detailed programming language knowledge is required. |
| 6 | Software testing is a vital phase of SDLC. | It is not a part of SDLC because it occurs as a subset of testing. |
| 7 | Testing is initiated after the coding phase or code is written. | Debugging commences with the execution of a test case. |
| 8 | Tester can plan, design & implement the testing process. | The debugging process cannot be planned. |
| 9 | Testing is composed of the validation and verification of software. | The developers logically evaluate and remove the software errors. |

---

## 16. Software Quality

- The quality of software can be defined as the ability of the software to function as per user requirements.
- When it comes to software products, it must satisfy all the functionalities written down in the SRS document.

**Software Quality includes:**
1. **Good Design:** Good visualization design to attract users.
2. **Durability:** The software works without any issue for a long period of time.
3. **Consistency:** Software performs consistently on different platforms and other devices.
4. **Maintainability:** Capture and fix bugs quickly. New features are added easily.
5. **Value for money:** Customers & companies who make this app should feel that the money spent on this app has not been wasted.

### Software Quality Dimensions / Parameters
1. **Maintainability:** The ease with which software can be modified (adding features, enhancing features, fixing bugs, etc.).
2. **Portability:** The ability of software to be transferred easily from one location to another.
3. **Functionality:** The ability of software to carry out the functions as specified or desired.
4. **Performance:** The speed at which software performs under a particular load.
5. **Compatibility:** The suitability of software for use in different environments like different devices, operating systems and browsers.
6. **Usability:** The degree of the software's ease of use.
7. **Reliability:** The ability of software to perform a required function under stated conditions without any errors.
8. **Security:** The extent of protection of software against unauthorized access, invasion of privacy, theft, loss of data etc. Ex. OTP.

### Factors Affecting Software Quality
1. **Product Operation Factors:** Correctness, Reliability, Efficiency, Integrity, Usability.
2. **Product Revision Factors:** Maintainability, Flexibility, Testability.
3. **Product Transition Factors:** Portability, Reusability, Interoperability.

### Software Quality Metrics
- SQM ensures that the software product is of the highest quality and standard.

**1. Customer Problem Metrics**
- Measuring the problems encountered by the customers while using the product.
- PUM = Total problems reported by a customer ÷ Total number of license months

**2. Customer Satisfaction Metrics**
- Deals with the overall quality of the product & how satisfied a customer is with that product.
- It is measured by: Very Satisfied, Satisfied, Neutral, Dissatisfied, Very Dissatisfied.

**3. Software Maintenance Metrics**
- After completion of development & testing, the product is released in the market.
- During this interval: how many defects arrived at the customer environment?

---

## 17. Software Quality Management (SQM)

- Software Quality Management (SQM) refers to the complete process that ensures a software product is developed as per national and international standards like ANSI, IEEE and ISO.

**Need of Software Quality Management**
1. Delivering high-quality products on time.
2. Increases stakeholder faith in the product & company.
3. High-quality products always ensure customer satisfaction.

**Activities of Software Quality Management**

```
Software Quality Management
├── Quality Assurance
├── Quality Planning
└── Quality Control
```

### How to Achieve Software Quality?

**Quality Assurance**
- It assures that the system meets specified requirements and customer expectations.
- It defines standards and methodologies for a successful development process.
- It assures Correctness, Efficiency, Flexibility, Maintainability, Portability, Usability etc.

**Quality Control**
- It focuses on achieving & fulfilling quality parameters or quality goals as per customer requirements.
- It focuses on delivering the product on time with accurate cost.

**Quality Planning**
- Select applicable procedures and standards for a particular project and modify as required to develop a quality plan.

---

## 18. Quality Assurance vs Quality Control

| No | Quality Assurance | Quality Control |
| :---: | :--- | :--- |
| 1 | A procedure that focuses on providing assurance that the quality requested will be achieved. | A procedure that focuses on fulfilling the quality requested. |
| 2 | In order to meet the customer requirements, QA defines standards and methodologies. | QC confirms that the standards are followed while working on the product. |
| 3 | QA is process oriented. | QC is product oriented. |
| 4 | QA aims to prevent the defect. | QC aims to identify and fix defects. |
| 5 | It's a proactive measure. | It's a reactive measure. |
| 6 | It is performed before Quality Control. | It is performed only after QA activity is done. |
| 7 | It does not involve executing the program. | It always involves executing a program. |
| 8 | QA is involved in the full software development life cycle. | QC is involved in the full software testing life cycle. |
| 9 | All team members are responsible for QA. | The testing team is responsible for QC. |
| 10 | It is the procedure to create the deliverables. | It is the procedure to verify the deliverables. |
| 11 | Less time-consuming activity. | More time-consuming activity. |
| 12 | QA ensures that everything is executed in the right way, and that is why it falls under verification activity. | QC ensures that whatever we have done is as per the requirement, and that is why it falls under validation activity. |