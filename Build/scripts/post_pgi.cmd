::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: PGO Build Workflow:
:: - init_pgi.cmd
:: - build (with PGI instrumentation enabled)
:: * init_pgo.cmd
:: - build (using PGO profile)

set _LINK_=
set POGO_TYPE=

goto:eof