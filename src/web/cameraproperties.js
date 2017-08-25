function setCameraProperty( strVar, strValue )
{
    var variablesMap = { };
    variablesMap[strVar] = strValue;

    setCameraProperties( variablesMap );
}

function setCameraProperties( variablesMap )
{
    $.ajax( {
        type        : "POST",
        url         : "/camera/config",
        data        : JSON.stringify( variablesMap ),
        contentType : "application/json; charset=utf-8",
        dataType    : "json",
        async       : true,
        success: function( data )
        {
            if ( data.status != "OK" )
            {
                console.log( "Failed setting camera property: " + data.status );
                if ( data.property )
                {
                    console.log( "Property: " + data.property );
                }
            }
        },
        failure: function( errMsg )
        {
            console.log( errMsg );
        }
    } );
}

function handleBoolProperty( cb )
{
    setCameraProperty( cb.id, ( cb.checked ) ? 1 : 0 );
}

function handleRangeProperty( range )
{
    setCameraProperty( range.id, range.value );
}

function handleSelectionProperty( selection )
{
    setCameraProperty( selection.id, selection.value );
}
