Feature: Modifying a task's start date
As someone using tasks
I can modify the start date of tasks to today 
In order to have them plan from today

  @wip
  Scenario: Setting a date's start date to today makes it appear in the workday view
    Given I display the "Inbox" page
    And there is an item named "Create Sozi SVG" in the central list
    When I open the item in the editor
    And I change the editor start date to "2015-03-10"
    And I display the "Workday" page
    And I look at the central list
    And I list the items
    Then the list contains "Create Sozi SVG"

  @wip
  Scenario: Setting a date's start date to a date in the past makes it appear in the workday view
    Given I display the "Tags / Physics" page
    And there is an item named "Buy a cake" in the central list
    When I open the item in the editor
    And I change the editor start date to "2014-03-10"
    And I display the "Workday" page
    And I look at the central list
    And I list the items
    Then the list contains "Buy a cake"
