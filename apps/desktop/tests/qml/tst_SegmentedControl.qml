import QtQuick
import QtTest
import DevDialogue.App

TestCase {
    id: testCase

    name: "SegmentedControl"
    when: windowShown
    visible: true
    width: 480
    height: 240

    SegmentedControl {
        id: control
        anchors.centerIn: parent
        width: 320
        items: [
            { label: "One", value: "one" },
            { label: "Two", value: "two" },
            { label: "Three", value: "three" }
        ]
        currentValue: "two"
    }

    SignalSpy {
        id: spy
        target: control
        signalName: "valueSelected"
    }

    function test_selectedIndexFollowsCurrentValue() {
        compare(control.selectedIndex, 1)

        control.currentValue = "three"
        compare(control.selectedIndex, 2)
    }

    function test_unknownValueHasNoSelection() {
        control.currentValue = "missing"
        compare(control.selectedIndex, -1)

        control.currentValue = "one"
        compare(control.selectedIndex, 0)
    }

    function test_selectValueEmitsTheSelection() {
        spy.clear()

        control.selectValue("three")

        compare(spy.count, 1)
        compare(spy.signalArguments[0][0], "three")
    }

    function test_segmentsAreLaidOutForEveryItem() {
        compare(control.items.length, 3)
        verify(control.implicitWidth > 0)
        verify(control.implicitHeight > 0)
    }
}
