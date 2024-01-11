# custom packages
from framework.helpers import logging_helper as lh
from framework.helpers import modules_helper as mh
from framework.helpers.dotdict import dotdict

# core packages
import itertools, csv
from tabulate import tabulate
from pathlib import Path

# data & machine learning packages
import pandas as pd
import seaborn as sns
import matplotlib
import matplotlib.pyplot as plt
from tqdm import tqdm

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# object for analyzining the training data
class TrainingDataAnalyzer(object):
    def __init__(self, config, data_generator, model_builder_class):
        self.config = config
        self.network_params = self.config.network
        self.analyze_params = self.config.analyze
        self.data_generator = data_generator
        self.model_builder_class = model_builder_class

    def _get_training_data_analyze_out_folder(self, dataset_name):
        return "Data/Train/_analyze/{module}/{datafile}/".format(
            module=mh.get_main_module(),
            datafile=dataset_name)

    def _create_training_data_plot(self, axis, dataset, axis_df, out_folder, out_filename):
        # plot the data axis
        g = sns.displot(
            data=axis_df,
            kind='hist',
            stat='probability',
            bins=self.analyze_params.plots.bins,
            legend=False)

        # set the plot title
        title = '{axis} ({dataset})'.format(axis=axis, dataset=dataset)
        title = g.fig.axes[0].set_title(title)

        # set the plot legend
        legend = g.fig.axes[0].legend(labels=axis_df.columns, ncol=1, fontsize='xx-small',
            bbox_to_anchor=(1.0, 0.5), loc='center left')
            
        # save the figure
        if self.analyze_params.export.plots:
            full_filename = "{folder}/{filename}.png".format(
                folder=out_folder,
                filename=out_filename)    
            Path(full_filename).parent.mkdir(parents=True, exist_ok=True)
            g.savefig(
                fname=full_filename, 
                format='png', 
                dpi=self.analyze_params.plots.save_dpi)
            
        # show the figure
        if self.analyze_params.plots.show:
            if matplotlib.get_backend() == 'Qt5Agg':
                fig_manager = plt.get_current_fig_manager()
                fig_manager.window.showMaximized()
            plt.tight_layout()
            plt.show()

    def _create_training_data_corr_plot(self, dataset, dataset_df, out_folder, out_filename, corr_columns=None):
        corr = dataset_df.corr()
        if corr_columns is not None:
            corr = dataset_df.corr().loc[corr_columns[0], corr_columns[1]]

        # plot the data axis
        ax = sns.heatmap(
            data=corr,
            annot=False)

        # set the plot title
        title = 'Correllation of variables for {dataset}'.format(dataset=dataset)
        title = ax.set_title(title)
            
        # save the figure 
        if self.analyze_params.export.plots:     
            full_filename = "{folder}/{filename}.png".format(
                folder=out_folder,
                filename=out_filename)
            Path(full_filename).parent.mkdir(parents=True, exist_ok=True)
            ax.savefig(
                fname=full_filename, 
                format='png', 
                dpi=self.analyze_params.plots.save_dpi)

        # show the figure
        if self.analyze_params.plots.show:
            if matplotlib.get_backend() == 'Qt5Agg':
                fig_manager = plt.get_current_fig_manager()
                fig_manager.window.showMaximized()
            plt.tight_layout()
            plt.show()

    def _eval_outlier_condition(self, dataset, row_id, cond_var, cond_props):
        # extract the appropriate column
        if cond_var in dataset.features:
            cond_var_value = dataset.features.iloc[row_id][cond_var]
        elif cond_var in dataset.targets:
            cond_var_value = dataset.targets.iloc[row_id][cond_var]
        else:
            raise ValueError('Unknown column name {}.'.format(cond_var))

        # apply abs if needed
        cond_var_sign = cond_props.sign
        if cond_var_sign == "real" or cond_var_sign == "true":
            pass
        elif cond_var_sign == "abs" or cond_var_sign == "absolute":
            cond_var_value = abs(cond_var_value)

        # apply the proper threshold
        cond_threshold = cond_props.threshold
        cond_relation = cond_props.relation
        if cond_relation == "greater":
            return cond_var_value >= cond_threshold
        if cond_relation == "greater_equal":
            return cond_var_value > cond_threshold
        elif cond_relation == "less":
            return cond_var_value < cond_threshold
        elif cond_relation == "less_equal":
            return cond_var_value <= cond_threshold
        else:
            raise ValueError('Unknown relation {}.'.format(cond_relation))

    # TODO: consider the param role ('eye', 'aberration', etc.) when generating MATLAB scripts
    def _log_dataset_outliers(self, dataset_name, dataset, out_folder):
        logger.info("Looking for outliers in dataset '{}'...", dataset_name)

        # go through each sample
        for i in tqdm(range(dataset.features.shape[0]), 'Evaluating samples'):
            # evaluate the test conditions
            conditions = [ ]
            for cond_var, cond_props in self.analyze_params.log_outliers.conditions.items():
                conditions.append(self._eval_outlier_condition(dataset, i, cond_var, cond_props))

            if all(conditions):
                if self.analyze_params.log_outliers.mat_scripts:
                    full_filename = "{folder}/outliers/{dataset}_{sample}.txt".format(
                        folder=out_folder,
                        dataset=dataset_name, 
                        sample=i)
                    
                    Path(full_filename).parent.mkdir(parents=True, exist_ok=True)
                    with open(full_filename, 'w') as out_file:
                        for xi, feature in enumerate(dataset.features.columns):
                            out_file.write('eye.{feature} = {value};\n'.format(feature=feature, value=dataset.features.iloc[i][xi]))
                        out_file.write('aberrations = {aberrations};\n'.format(aberrations=dataset.targets.iloc[i].to_list()))

    def analyze_dataframes(self, training_data, out_folder, dataframes, axis_name):
        properties = [ 'mean', 'abs. mean', 'std.', 'min', 'max', 'max/std', 'q1', 'q2', 'q3' ]
        headers_interleaved = [ '{} ({})'.format(dataset, property) for dataset, property in itertools.product(self.analyze_params.data.datasets, properties) ]
        headers = itertools.chain([ 'Name' ], headers_interleaved)
        column_names_list = [ list(df.columns) for df in dataframes ]
        column_names = list(itertools.chain(*column_names_list))
        values = [ itertools.chain([ '> Global' ], column_names) ]

        for dataframe in dataframes:
            dataframe_stacked = dataframe.stack()

            # compute the per-column metrics
            mean_per_feature = dataframe.mean(axis=0)
            mean_abs_per_feature = dataframe.abs().mean(axis=0)
            std_per_feature = dataframe.std(axis=0)
            min_per_feature = dataframe.min(axis=0)
            max_per_feature = dataframe.max(axis=0)
            max_std_per_feature = (dataframe.abs().max(axis=0) - dataframe.abs().mean(axis=0)) / dataframe.std(axis=0)
            q1_per_feature = dataframe.quantile(q=0.25, axis=0)
            q2_per_feature = dataframe.quantile(q=0.5, axis=0)
            q3_per_feature = dataframe.quantile(q=0.75, axis=0)

            # compute the global metrics
            mean = mean_per_feature.mean()
            mean_abs = mean_abs_per_feature.mean()
            std = dataframe_stacked.std()
            min = dataframe_stacked.min()
            max = dataframe_stacked.max()
            max_std = max_std_per_feature.mean()
            q1 = dataframe_stacked.quantile(q=0.25)
            q2 = dataframe_stacked.quantile(q=0.5)
            q3 = dataframe_stacked.quantile(q=0.75)

            # put them into a tabular data        
            values.append(itertools.chain([ mean ], mean_per_feature.values))
            values.append(itertools.chain([ mean_abs ], mean_abs_per_feature.values))
            values.append(itertools.chain([ std ], std_per_feature.values))
            values.append(itertools.chain([ min ], min_per_feature.values))
            values.append(itertools.chain([ max ], max_per_feature.values))
            values.append(itertools.chain([ max_std ], max_std_per_feature.values))
            values.append(itertools.chain([ q1 ], q1_per_feature.values))
            values.append(itertools.chain([ q2 ], q2_per_feature.values))
            values.append(itertools.chain([ q3 ], q3_per_feature.values))

        values = list(zip(*values))
        tabulated_metrics = tabulate(values, headers=headers, tablefmt='psql')

        # print them
        logger.info('Per-column metrics for {}:\n{}', axis_name, tabulated_metrics)

        # export the metrics
        if self.analyze_params.export.metrics:
            # write them out as well
            full_filename = "{folder}/{axis}.txt".format(
                folder=out_folder, 
                axis=axis_name)

            Path(full_filename).parent.mkdir(parents=True, exist_ok=True)
            with open(full_filename, 'w') as out_file:
                out_file.write(tabulated_metrics)

            csv_filename = "{folder}/{axis}.csv".format(
                folder=out_folder, 
                axis=axis_name)

            Path(csv_filename).parent.mkdir(parents=True, exist_ok=True)

            csv_headers = [ 'name', 'mean', 'abs_mean', 'std', 'min', 'max', 'max_over_std', 'q1', 'q2', 'q3' ]
            with open(csv_filename, 'w', newline='', encoding='utf-8') as csv_file:
                csv_writer = csv.writer(csv_file, delimiter=';')
                csv_writer.writerow(csv_headers)
                for row in values:
                    csv_writer.writerow(list(row))

    def analyze(self):
        logger.info('Displaying training data statistics...')

        # instantiate the model builder
        model = self.model_builder_class(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=self.network_params)

        # prepare the training data
        training_data = model.prepare_training_data(
            training_type=TrainingType.Train)

        # generate the output folder
        out_folder = self._get_training_data_analyze_out_folder(training_data.dataset_name)

        # list of datasets to evaluate
        datasets = {
            'full': dotdict({
                'features': training_data.features, 
                'targets': training_data.targets }),
            'train': dotdict({
                'features': training_data.x_train, 
                'targets': training_data.y_train }),
            'eval': dotdict({
                'features': training_data.x_eval, 
                'targets': training_data.y_eval }),
            'train_normalized': dotdict({
                'features': training_data.x_train_normalized, 
                'targets': training_data.y_train_normalized }),
            'eval_normalized': dotdict({
                'features': training_data.x_eval_normalized, 
                'targets': training_data.y_eval_normalized }),
        }
        
        # list of columns to analyze
        column_names = {
            'features': training_data.feature_names,
            'targets': training_data.target_names
        }

        # export the full datasets
        if self.analyze_params.export.dataset:
            dataset_df = pd.DataFrame()
            for dataset in self.analyze_params.data.datasets:
                axis_df = pd.DataFrame(datasets[dataset][axis], columns=column_names[axis])
                dataset_df = pd.concat([dataset_df, axis_df], ignore_index=True)
                
            data_csv_filename = "{folder}/dataset_{axis}.csv".format(
                folder=out_folder, 
                axis=axis)
            Path(data_csv_filename).parent.mkdir(parents=True, exist_ok=True)

            dataset_df.to_csv(data_csv_filename)

        for axis in self.analyze_params.data.axes:
            dataframes = [ pd.DataFrame(datasets[dataset][axis], columns=column_names[axis]) for dataset in self.analyze_params.data.datasets ]
            self.analyze_dataframes(training_data, out_folder, dataframes, axis)

        for group_name, group_columns in self.analyze_params.data.groups.items():
            dataframes = []
            for dataset in self.analyze_params.data.datasets:
                for axis_name, axis_columns in column_names.items():
                    columns = [ column for column in group_columns if column in axis_columns ]
                    if len(columns) != 0:
                        df = pd.DataFrame(datasets[dataset][axis_name], columns=columns)
                        dataframes.append(df)
            if len(dataframes) != 0:
                self.analyze_dataframes(training_data, out_folder, dataframes, group_name)

        # create distribution plots
        if "distribution" in self.analyze_params.plots.metrics:
            for dataset in self.analyze_params.data.datasets:
                for axis in self.analyze_params.data.axes:
                    axis_df = pd.DataFrame(datasets[dataset][axis], columns=column_names[axis])
                    filename = "{dataset}_{axis}_distribution".format(dataset=dataset, axis=axis)
                    self._create_training_data_plot(
                        axis=axis,
                        dataset=dataset,
                        axis_df=axis_df,
                        out_folder=out_folder,
                        out_filename=filename)
        
        # create correllation plots
        if "correlation" in self.analyze_params.plots.metrics:
            for dataset in self.analyze_params.data.datasets:
                for axis in self.analyze_params.data.axes:
                    axis_df = pd.DataFrame(datasets[dataset][axis], columns=column_names[axis])
                    filename = "{dataset}_{axis}_correlation".format(dataset=dataset, axis=axis)
                    self._create_training_data_corr_plot(
                        dataset=dataset,
                        dataset_df=axis_df,
                        out_folder=out_folder,
                        out_filename=filename)
        
        if "cross_correlation" in self.analyze_params.plots.metrics:
            for dataset in self.analyze_params.data.datasets:
                axes = list(datasets[dataset].keys())
                ax_cols = list(map(lambda ax: column_names[ax], axes))
                ax_dfs = list(map(lambda ax: pd.DataFrame(datasets[dataset][ax], columns=column_names[ax]), axes))
                cross_df = ax_dfs[0].join(ax_dfs[1:])
                title = "cross-correlation between {ax1}-{ax2}".format(ax1=axes[0], ax2=axes[1])
                filename = "{dataset}_{ax1}-{ax2}_cross-correlation".format(dataset=dataset, ax1=axes[0], ax2=axes[1])
                self._create_training_data_corr_plot(
                    dataset=title,
                    dataset_df=cross_df,
                    corr_columns=ax_cols,
                    out_folder=out_folder,
                    out_filename=filename)

        # log outliers
        if 'log_outliers' in self.analyze_params and (self.analyze_params.log_outliers.sum_file or self.analyze_params.log_outliers.mat_scripts):
            for dataset in self.analyze_params.data.datasets:
                self._log_dataset_outliers(
                    dataset_name=dataset,
                    dataset=datasets[dataset],
                    out_folder=out_folder)