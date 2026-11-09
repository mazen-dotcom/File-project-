namespace Query::Tokens
{
    enum class Table
    {
        Doctors,
        Appointments,
        Both
    };

    enum class Field
    {
        All,             // "all"
        DoctorID,        // "Doctor ID"
        DoctorName,      // "Doctor Name"
        DoctorSpecialty, // "Doctor Specialty"
        Address,         // "Address"
        AppointmentID,   // "Appointment ID"
        AppointmentDate, // "Appointment Date"
        Invalid
    };

    enum class Op
    {
        Equals,
        NotEquals,
        Contains,
        StartsWith,
        EndsWith,
        GreaterThan,
        LessThan,
        GreaterOrEqual,
        LessOrEqual
    };

    enum class LogicOp
    {
        None,
        And, // "and"
        Or   // "or"
    };

    // Helper enum to classify which index strategy to use during execution
    enum class SearchStrategy
    {
        PrimaryIndex,
        SecondaryIndex,
        MultipleSecondaryIndices,
        LinearSearch
    };

    inline bool isValidField(Table table, Field field)
    {
        if (field == Field::All)
            return true;

        switch (table)
        {
            case Table::Doctors:
                return field == Field::DoctorID ||
                    field == Field::DoctorName ||
                    field == Field::DoctorSpecialty ||
                    field == Field::Address;

            case Table::Appointments:
                return field == Field::DoctorID ||
                    field == Field::AppointmentID ||
                    field == Field::AppointmentDate;

            case Table::Both:
                return true;

            default:
                return false;
        }
    }
}