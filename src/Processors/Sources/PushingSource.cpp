#include <Processors/Sources/PushingSource.h>

namespace DB
{
namespace ErrorCodes
{
    extern const int LOGICAL_ERROR;
}

ISource::Status PushingSource::prepare()
{
    if (finished || isCancelled())
    {
        output.finish();
        return Status::Finished;
    }

    /// Check can output.
    if (output.isFinished())
        return Status::Finished;

    if (!output.canPush())
        return Status::PortFull;

    if (!has_input)
    {
        if (!pushed_input)
        {
             //std::cerr << "yielding\n";
             no_input_flag = true;
        }
        //std::cerr << "ready\n";
        return Status::Ready;
    }

    output.pushData(std::move(current_chunk));
    has_input = false;

    if (got_exception)
    {
        finished = true;
        output.finish();
        return Status::Finished;
    }

    /// Now, we pushed to output, and it must be full.
    return Status::PortFull;
}

void PushingSource::push(const Block & block)
{
    //std::cerr << "Pushing\n";
    if (chunk)
        throw Exception("Cannot push block to a non-empty PushingSource", ErrorCodes::LOGICAL_ERROR);

    pushed_input = true;
    no_input_flag = false;

    if (!block)
        return;

    Columns columns;
    columns.reserve(column_names.size());

    for (const auto & name : column_names)
        columns.push_back(block.getByName(name).column);

    chunk = Chunk(std::move(columns), block.rows());
}

}